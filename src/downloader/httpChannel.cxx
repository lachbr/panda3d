// Filename: httpChannel.cxx
// Created by:  drose (24Sep02)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://www.panda3d.org/license.txt .
//
// To contact the maintainers of this program write to
// panda3d@yahoogroups.com .
//
////////////////////////////////////////////////////////////////////

#include "httpChannel.h"
#include "httpClient.h"
#include "bioStream.h"
#include "chunkedStream.h"
#include "identityStream.h"
#include "config_downloader.h"
#include "clockObject.h"
#include "buffer.h"  // for Ramfile

#ifdef HAVE_SSL
#ifdef REPORT_OPENSSL_ERRORS
#include <openssl/err.h>
#endif

TypeHandle HTTPChannel::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::Constructor
//       Access: Private
//  Description: 
////////////////////////////////////////////////////////////////////
HTTPChannel::
HTTPChannel(HTTPClient *client) :
  _client(client)
{
  _persistent_connection = false;
  _connect_timeout = connect_timeout;
  _blocking_connect = false;
  _download_throttle = false;
  _max_bytes_per_second = downloader_byte_rate;
  _seconds_per_update = downloader_frequency;
  _max_updates_per_second = 1.0f / _seconds_per_update;
  _bytes_per_update = int(_max_bytes_per_second * _seconds_per_update);
  _nonblocking = false;
  _want_ssl = false;
  _proxy_serves_document = false;
  _first_byte = 0;
  _last_byte = 0;
  _read_index = 0;
  _file_size = 0;
  _bytes_downloaded = 0;
  _bytes_requested = 0;
  _status_code = 0;
  _status_string = string();
  _response_type = RT_none;
  _proxy = _client->get_proxy();
  _http_version = _client->get_http_version();
  _http_version_string = _client->get_http_version_string();
  _state = S_new;
  _done_state = S_new;
  _started_download = false;
  _sent_so_far = 0;
  _proxy_tunnel = false;
  _body_stream = NULL;
  _sbio = NULL;
  _last_status_code = 0;
  _last_run_time = 0.0f;
  _download_to_ramfile = NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
HTTPChannel::
~HTTPChannel() {
  close_connection();
  reset_download_to();
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::get_file_system
//       Access: Published, Virtual
//  Description: Returns the VirtualFileSystem this file is associated
//               with.
////////////////////////////////////////////////////////////////////
VirtualFileSystem *HTTPChannel::
get_file_system() const {
  return NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::get_filename
//       Access: Published, Virtual
//  Description: Returns the full pathname to this file within the
//               virtual file system.
////////////////////////////////////////////////////////////////////
Filename HTTPChannel::
get_filename() const {
  return Filename();
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::is_regular_file
//       Access: Published, Virtual
//  Description: Returns true if this file represents a regular file
//               (and read_file() may be called), false otherwise.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
is_regular_file() const {
  return is_valid();
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::will_close_connection
//       Access: Public
//  Description: Returns true if the server has indicated it will
//               close the connection after this document has been
//               read, or false if it will remain open (and future
//               documents may be requested on the same connection).
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
will_close_connection() const {
  if (get_http_version() < HTTPEnum::HV_11) {
    // pre-HTTP 1.1 always closes.
    return true;
  }

  string connection = get_header_value("Connection");
  if (downcase(connection) == "close") {
    // The server says it will close.
    return true;
  }

  // Assume the server will keep it open.
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::open_read_file
//       Access: Public, Virtual
//  Description: Opens the document for reading.  Returns a newly
//               allocated istream on success (which you should
//               eventually delete when you are done reading).
//               Returns NULL on failure.  This may only be called
//               once for a particular HTTPChannel.
////////////////////////////////////////////////////////////////////
istream *HTTPChannel::
open_read_file() const {
  return ((HTTPChannel *)this)->read_body();
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::get_header_value
//       Access: Published
//  Description: Returns the HTML header value associated with the
//               indicated key, or empty string if the key was not
//               defined in the message returned by the server.
////////////////////////////////////////////////////////////////////
string HTTPChannel::
get_header_value(const string &key) const {
  Headers::const_iterator hi = _headers.find(downcase(key));
  if (hi != _headers.end()) {
    return (*hi).second;
  }
  return string();
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::write_headers
//       Access: Published
//  Description: Outputs a list of all headers defined by the server
//               to the indicated output stream.
////////////////////////////////////////////////////////////////////
void HTTPChannel::
write_headers(ostream &out) const {
  Headers::const_iterator hi;
  for (hi = _headers.begin(); hi != _headers.end(); ++hi) {
    out << (*hi).first << ": " << (*hi).second << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run
//       Access: Published
//  Description: This must be called from time to time when
//               non-blocking I/O is in use.  It checks for data
//               coming in on the socket and writes data out to the
//               socket when possible, and does whatever processing is
//               required towards completing the current task.
//
//               The return value is true if the task is still pending
//               (and run() will need to be called again in the
//               future), or false if the current task is complete.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run() {
  if (_state == _done_state || _state == S_failure) {
    clear_extra_headers();
    if (!reached_done_state()) {
      return false;
    }
  }

  if (_started_download) {
    if (_nonblocking && _download_throttle) {
      double now = ClockObject::get_global_clock()->get_real_time();
      double elapsed = now - _last_run_time;
      if (elapsed < _seconds_per_update) {
        // Come back later.
        return true;
      }
      int num_potential_updates = (int)(elapsed / _seconds_per_update);
      _last_run_time = now;
      _bytes_requested += _bytes_per_update * num_potential_updates;
      if (downloader_cat.is_spam()) {
        downloader_cat.spam()
          << "elapsed = " << elapsed << " num_potential_updates = " 
          << num_potential_updates << " bytes_requested = " 
          << _bytes_requested << "\n";
      }
    }
    switch (_download_dest) {
    case DD_none:
      return false;  // We're done.

    case DD_file:
      return run_download_to_file();

    case DD_ram:
      return run_download_to_ram();
    }
  }

  if (downloader_cat.is_spam()) {
    downloader_cat.spam()
      << "begin run(), _state = " << (int)_state << ", _done_state = "
      << (int)_done_state << "\n";
  }

  if (_state == _done_state) {
    return reached_done_state();
  }

  bool repeat_later;
  do {
    if (_bio.is_null()) {
      // No connection.  Attempt to establish one.
      _proxy = _client->get_proxy();
      
      if (_proxy.empty()) {
        _bio = new BioPtr(_url);
      } else {
        _bio = new BioPtr(_proxy);
      }
      _source = new BioStreamPtr(new BioStream(_bio));
      if (_nonblocking) {
        BIO_set_nbio(*_bio, 1);
      }
      
      _state = S_connecting;
      _started_connecting_time = 
        ClockObject::get_global_clock()->get_real_time();
    }

    if (downloader_cat.is_spam()) {
      downloader_cat.spam()
        << "continue run(), _state = " << (int)_state << "\n";
    }

    switch (_state) {
    case S_connecting:
      repeat_later = run_connecting();
      break;
      
    case S_connecting_wait:
      repeat_later = run_connecting_wait();
      break;
      
    case S_proxy_ready:
      repeat_later = run_proxy_ready();
      break;
      
    case S_proxy_request_sent:
      repeat_later = run_proxy_request_sent();
      break;
      
    case S_proxy_reading_header:
      repeat_later = run_proxy_reading_header();
      break;
      
    case S_setup_ssl:
      repeat_later = run_setup_ssl();
      break;
      
    case S_ssl_handshake:
      repeat_later = run_ssl_handshake();
      break;
      
    case S_ready:
      repeat_later = run_ready();
      break;
      
    case S_request_sent:
      repeat_later = run_request_sent();
      break;
      
    case S_reading_header:
      repeat_later = run_reading_header();
      break;
      
    case S_read_header:
      repeat_later = run_read_header();
      break;
      
    case S_begin_body:
      repeat_later = run_begin_body();
      break;
      
    case S_reading_body:
      repeat_later = run_reading_body();
      break;

    case S_read_body:
      repeat_later = run_read_body();
      break;

    case S_read_trailer:
      repeat_later = run_read_trailer();
      break;
      
    default:
      downloader_cat.warning()
        << "Unhandled state " << (int)_state << "\n";
      return false;
    }

    if (_state == _done_state || _state == S_failure) {
      clear_extra_headers();
      // We've reached our terminal state.
      return reached_done_state();
    }
  } while (!repeat_later || _bio.is_null());

  if (downloader_cat.is_spam()) {
    downloader_cat.spam()
      << "later run(), _state = " << (int)_state
      << ", _done_state = " << (int)_done_state << "\n";
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::read_body
//       Access: Published
//  Description: Returns a newly-allocated istream suitable for
//               reading the body of the document.  This may only be
//               called immediately after a call to get_document() or
//               post_form(), or after a call to run() has returned
//               false.
//
//               The user is responsible for deleting the returned
//               istream later.
////////////////////////////////////////////////////////////////////
ISocketStream *HTTPChannel::
read_body() {
  if ((_state != S_read_header && _state != S_begin_body) || _source.is_null()) {
    return NULL;
  }

  string transfer_coding = downcase(get_header_value("Transfer-Encoding"));
  string content_length = get_header_value("Content-Length");

  ISocketStream *result;
  if (transfer_coding == "chunked") {
    // "chunked" transfer encoding.  This means we will have to decode
    // the length of the file as we read it in chunks.  The
    // IChunkedStream does this.
    _file_size = 0;
    _state = S_reading_body;
    _read_index++;
    result = new IChunkedStream(_source, (HTTPChannel *)this);

  } else {
    // If the transfer encoding is anything else, assume "identity".
    // This is just the literal characters following the header, up
    // until _file_size bytes have been read (if content-length was
    // specified), or till end of file otherwise.
    _state = S_reading_body;
    _read_index++;
    result = new IIdentityStream(_source, (HTTPChannel *)this, 
                                 !content_length.empty(), _file_size);
  }

  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::download_to_file
//       Access: Published
//  Description: Specifies the name of a file to download the
//               resulting document to.  This should be called
//               immediately after get_document() or
//               begin_get_document() or related functions.
//
//               In the case of the blocking I/O methods like
//               get_document(), this function will download the
//               entire document to the file and return true if it was
//               successfully downloaded, false otherwise.
//
//               In the case of non-blocking I/O methods like
//               begin_get_document(), this function simply indicates an
//               intention to download to the indicated file.  It
//               returns true if the file can be opened for writing,
//               false otherwise, but the contents will not be
//               completely downloaded until run() has returned false.
//               At this time, it is possible that a communications
//               error will have left a partial file, so
//               is_download_complete() may be called to test this.
//
//               If subdocument_resumes is true and the document in
//               question was previously requested as a subdocument
//               (i.e. get_subdocument() with a first_byte value
//               greater than zero), this will automatically seek to
//               the appropriate byte within the file for writing the
//               output.  In this case, the file must already exist
//               and must have at least first_byte bytes in it.  If
//               subdocument_resumes is false, a subdocument will
//               always be downloaded beginning at the first byte of
//               the file.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
download_to_file(const Filename &filename, bool subdocument_resumes) {
  reset_download_to();
  _download_to_filename = filename;
  _download_to_filename.set_binary();
  _download_to_file.close();
  _download_to_file.clear();

  _subdocument_resumes = (subdocument_resumes && _first_byte != 0);

  if (!_download_to_filename.open_write(_download_to_file, !_subdocument_resumes)) {
    downloader_cat.info()
      << "Could not open " << _download_to_filename << " for writing.\n";
    return false;
  }

  _download_dest = DD_file;
  if (!reset_download_position()) {
    reset_download_to();
    return false;
  }

  if (_nonblocking) {
    // In nonblocking mode, we can't start the download yet; that will
    // be done later as run() is called.
    return true;
  }

  // In normal, blocking mode, go ahead and do the download.
  run();
  return is_download_complete();
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::download_to_ram
//       Access: Published
//  Description: Specifies a Ramfile object to download the
//               resulting document to.  This should be called
//               immediately after get_document() or
//               begin_get_document() or related functions.
//
//               In the case of the blocking I/O methods like
//               get_document(), this function will download the
//               entire document to the Ramfile and return true if it
//               was successfully downloaded, false otherwise.
//
//               In the case of non-blocking I/O methods like
//               begin_get_document(), this function simply indicates an
//               intention to download to the indicated Ramfile.  It
//               returns true if the file can be opened for writing,
//               false otherwise, but the contents will not be
//               completely downloaded until run() has returned false.
//               At this time, it is possible that a communications
//               error will have left a partial file, so
//               is_download_complete() may be called to test this.
//
//               If subdocument_resumes is true and the document in
//               question was previously requested as a subdocument
//               (i.e. get_subdocument() with a first_byte value
//               greater than zero), this will automatically seek to
//               the appropriate byte within the Ramfile for writing
//               the output.  In this case, the Ramfile must already
//               have at least first_byte bytes in it.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
download_to_ram(Ramfile *ramfile, bool subdocument_resumes) {
  nassertr(ramfile != (Ramfile *)NULL, false);
  reset_download_to();
  ramfile->_pos = 0;
  _download_to_ramfile = ramfile;
  _download_dest = DD_ram;
  _subdocument_resumes = (subdocument_resumes && _first_byte != 0);

  if (!reset_download_position()) {
    reset_download_to();
    return false;
  }

  if (_nonblocking) {
    // In nonblocking mode, we can't start the download yet; that will
    // be done later as run() is called.
    return true;
  }

  // In normal, blocking mode, go ahead and do the download.
  run();
  return is_download_complete();
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::get_connection
//       Access: Published
//  Description: Returns the connection that was established via a
//               previous call to connect_to() or begin_connect_to(),
//               or NULL if the connection attempt failed or if those
//               methods have not recently been called.
//
//               This stream has been allocated from the free store.
//               It is the user's responsibility to delete this
//               pointer when finished with it.
////////////////////////////////////////////////////////////////////
SocketStream *HTTPChannel::
get_connection() {
  if (!is_connection_ready()) {
    return NULL;
  }

  BioStream *stream = _source->get_stream();
  _source->set_stream(NULL);

  // We're now passing ownership of the connection to the user.
  reset_to_new();

  return stream;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::downcase
//       Access: Public, Static
//  Description: Returns the input string with all uppercase letters
//               converted to lowercase.
////////////////////////////////////////////////////////////////////
string HTTPChannel::
downcase(const string &s) {
  string result;
  result.reserve(s.size());
  string::const_iterator p;
  for (p = s.begin(); p != s.end(); ++p) {
    result += tolower(*p);
  }
  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::reached_done_state
//       Access: Private
//  Description: Called by run() after it reaches the done state, this
//               simply checks to see if a download was requested, and
//               begins the download if it has been.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
reached_done_state() {
  if (downloader_cat.is_spam()) {
    downloader_cat.spam()
      << "terminating run(), _state = " << (int)_state
      << ", _done_state = " << (int)_done_state << "\n";
  }

  if (_state == S_failure || _download_dest == DD_none) {
    // All done.
    return false;
    
  } else {
    // Oops, we have to download the body now.
    _body_stream = read_body();
    if (_body_stream == (ISocketStream *)NULL) {
      if (downloader_cat.is_debug()) {
        downloader_cat.debug()
          << "Unable to download body.\n";
      }
      return false;
    } else {
      _started_download = true;
      _last_run_time = ClockObject::get_global_clock()->get_real_time();
      return true;
    }
  }
}
  
////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_connecting
//       Access: Private
//  Description: In this state, we have not yet established a
//               network connection to the server (or proxy).
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_connecting() {
  _status_code = 0;
  _status_string = string();
  if (BIO_do_connect(*_bio) <= 0) {
    if (BIO_should_retry(*_bio)) {
      _state = S_connecting_wait;
      return false;
    }
    downloader_cat.info()
      << "Could not connect to " << _bio->get_server_name() << ":" 
      << _bio->get_port() << "\n";
#ifdef REPORT_OPENSSL_ERRORS
    ERR_print_errors_fp(stderr);
#endif
    _state = S_failure;
    return false;
  }

  if (downloader_cat.is_debug()) {
    downloader_cat.debug()
      << "Connected to " << _bio->get_server_name() << ":" 
      << _bio->get_port() << "\n";
  }

  if (!_proxy.empty()) {
    _state = S_proxy_ready;

  } else {
    _state = _want_ssl ? S_setup_ssl : S_ready;
  }
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_connecting_wait
//       Access: Private
//  Description: Here we have begun to establish a nonblocking
//               connection, but we got a come-back-later message, so
//               we are waiting for the socket to finish connecting.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_connecting_wait() {
  int fd = -1;
  BIO_get_fd(*_bio, &fd);
  if (fd < 0) {
    downloader_cat.warning()
      << "nonblocking socket BIO has no file descriptor.\n";
    _state = S_failure;
    return false;
  }

  if (downloader_cat.is_debug()) {
    downloader_cat.debug()
      << "waiting to connect to " << _url.get_server() << ":" 
      << _url.get_port() << ".\n";
  }
  fd_set wset;
  FD_ZERO(&wset);
  FD_SET(fd, &wset);
  struct timeval tv;
  if (get_blocking_connect()) {
    // Since we'll be blocking on this connect, fill in the timeout
    // into the structure.
    tv.tv_sec = (int)_connect_timeout;
    tv.tv_usec = (int)((_connect_timeout - tv.tv_sec) * 1000000.0);
  } else {
    // We won't block on this connect, so select() for 0 time.
    tv.tv_sec = 0;
    tv.tv_usec = 0;
  }
  int errcode = select(fd + 1, NULL, &wset, NULL, &tv);
  if (errcode < 0) {
    downloader_cat.warning()
      << "Error in select.\n";
    _state = S_failure;
    return false;
  }
  
  if (errcode == 0) {
    // Nothing's happened so far; come back later.
    if (get_blocking_connect() ||
        (ClockObject::get_global_clock()->get_real_time() - 
         _started_connecting_time > get_connect_timeout())) {
      // Time to give up.
      downloader_cat.info()
        << "Timeout connecting to " << _url.get_server() << ":" 
        << _url.get_port() << ".\n";
      _state = S_failure;
      return false;
    }
    return true;
  }
  
  // The socket is now ready for writing.
  _state = S_connecting;
  return false;
}


////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_proxy_ready
//       Access: Private
//  Description: This state is reached only after first establishing a
//               connection to the proxy, if a proxy is in use.
//
//               In the normal http mode, this state immediately
//               transitions to S_ready, but in some cases (like
//               https-over-proxy) we need to send a special message
//               directly to the proxy that is separate from the http
//               request we will send to the server.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_proxy_ready() {
  // If there's a request to be sent to the proxy, send it now.
  if (!_proxy_request_text.empty()) {
    if (!http_send(_proxy_request_text)) {
      return true;
    }
    
    // All done sending request.
    _state = S_proxy_request_sent;

  } else {
    _state = S_ready;
  }
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_proxy_request_sent
//       Access: Private
//  Description: This state is reached only after we have sent a
//               special message to the proxy and we are waiting for
//               the proxy's response.  It is not used in the normal
//               http-over-proxy case, which does not require a
//               special message to the proxy.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_proxy_request_sent() {
  // Wait for the first line to come back from the server.
  string line;
  if (!http_getline(line)) {
    if (_bio.is_null()) {
      // Huh, the proxy hung up on us as soon as we tried to connect.
      if (_response_type == RT_hangup) {
        // This was our second immediate hangup in a row.  Give up.
        _state = S_failure;
        
      } else {
        // Try again, once.
        _response_type = RT_hangup;
      }
    }
    return true;
  }

  if (!parse_http_response(line)) {
    return false;
  }

  _state = S_proxy_reading_header;
  _current_field_name = string();
  _current_field_value = string();
  _headers.clear();
  _file_size = 0;
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_proxy_reading_header
//       Access: Private
//  Description: In this state we are reading the header lines from
//               the proxy's response to our special message.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_proxy_reading_header() {
  if (parse_http_header()) {
    return true;
  }

  _redirect = get_header_value("Location");

  _server_response_has_no_body = 
    (get_status_code() / 100 == 1 ||
     get_status_code() == 204 ||
     get_status_code() == 304);

  int last_status = _last_status_code;
  _last_status_code = get_status_code();

  if (get_status_code() == 407 && last_status != 407 && !_proxy.empty()) {
    // 407: not authorized to proxy.  Try to get the authorization.
    string authenticate_request = get_header_value("Proxy-Authenticate");
    _proxy_auth = _client->generate_auth(_proxy, true, authenticate_request);
    if (_proxy_auth != (HTTPAuthorization *)NULL) {
      _proxy_realm = _proxy_auth->get_realm();
      _proxy_username = _client->select_username(_proxy, true, _proxy_realm);
      if (!_proxy_username.empty()) {
        make_proxy_request_text();

        // Roll the state forward to force a new request.
        _state = S_begin_body;
        return false;
      }
    }
  }

  if (!is_valid()) {
    // Proxy wouldn't open connection.

    // Change some of the status codes a proxy might return to
    // differentiate them from similar status codes the destination
    // server might have returned.
    if (get_status_code() != 407) {
      _status_code += 1000;
    }

    _state = S_failure;
    return false;
  }

  // Now we have a tunnel opened through the proxy.
  _proxy_tunnel = true;
  make_request_text();

  _state = _want_ssl ? S_setup_ssl : S_ready;
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_setup_ssl
//       Access: Private
//  Description: This state begins elevating our existing, unsecure
//               connection to a secure, SSL connection.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_setup_ssl() {
  _sbio = BIO_new_ssl(_client->get_ssl_ctx(), true);
  BIO_push(_sbio, *_bio);

  if (downloader_cat.is_debug()) {
    downloader_cat.debug()
      << "performing SSL handshake\n";
  }
  _state = S_ssl_handshake;
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_ssl_handshake
//       Access: Private
//  Description: This state performs the SSL handshake with the
//               server, and also verifies the server's identity when
//               the handshake has successfully completed.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_ssl_handshake() {
  if (BIO_do_handshake(_sbio) <= 0) {
    if (BIO_should_retry(_sbio)) {
      return true;
    }
    downloader_cat.info()
      << "Could not establish SSL handshake with " 
      << _url.get_server() << ":" << _url.get_port() << "\n";
#ifdef REPORT_OPENSSL_ERRORS
    ERR_print_errors_fp(stderr);
#endif
    // It seems to be an error to free sbio at this point; perhaps
    // it's already been freed?
    _state = S_failure;
    return false;
  }

  SSL *ssl;
  BIO_get_ssl(_sbio, &ssl);
  nassertr(ssl != (SSL *)NULL, false);

  if (!_nonblocking) {
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
  }

  // Now that we've made an SSL handshake, we can use the SSL bio to
  // do all of our communication henceforth.
  _bio->set_bio(_sbio);
  _sbio = NULL;

  // Now verify the server is who we expect it to be.
  long verify_result = SSL_get_verify_result(ssl);
  if (verify_result == X509_V_ERR_CERT_HAS_EXPIRED) {
    downloader_cat.info()
      << "Expired certificate from " << _url.get_server() << ":"
      << _url.get_port() << "\n";
    if (_client->get_verify_ssl() == HTTPClient::VS_normal) {
      _state = S_failure;
      return false;
    }

  } else if (verify_result == X509_V_ERR_CERT_NOT_YET_VALID) {
    downloader_cat.info()
      << "Premature certificate from " << _url.get_server() << ":"
      << _url.get_port() << "\n";
    if (_client->get_verify_ssl() == HTTPClient::VS_normal) {
      _state = S_failure;
      return false;
    }

  } else if (verify_result != X509_V_OK) {
    downloader_cat.info()
      << "Unable to verify identity of " << _url.get_server() << ":" 
      << _url.get_port() << ", verify error code " << verify_result << "\n";
    if (_client->get_verify_ssl() != HTTPClient::VS_no_verify) {
      _state = S_failure;
      return false;
    }
  }

  X509 *cert = SSL_get_peer_certificate(ssl);
  if (cert == (X509 *)NULL) {
    downloader_cat.info()
      << "No certificate was presented by server.\n";
    if (_client->get_verify_ssl() != HTTPClient::VS_no_verify ||
        !_client->_expected_servers.empty()) {
      _state = S_failure;
      return false;
    }

  } else {
    if (downloader_cat.is_debug()) {
      downloader_cat.debug()
        << "Received certificate from server:\n" << flush;
      X509_print_fp(stderr, cert);
      fflush(stderr);
    }

    X509_NAME *subject = X509_get_subject_name(cert);

    if (downloader_cat.is_debug()) {
      string org_name = get_x509_name_component(subject, NID_organizationName);
      string org_unit_name = get_x509_name_component(subject, NID_organizationalUnitName);
      string common_name = get_x509_name_component(subject, NID_commonName);
      
      downloader_cat.debug()
        << "Server is " << common_name << " from " << org_unit_name
        << " / " << org_name << "\n";
    }

    if (!verify_server(subject)) {
      downloader_cat.info()
        << "Server does not match any expected server.\n";
      _state = S_failure;
      return false;
    }
      
    X509_free(cert);
  }

  _state = S_ready;
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_ready
//       Access: Private
//  Description: This is the main "ready" state.  In this state, we
//               have established a (possibly secure) connection to
//               the server (or proxy), and the server (or proxy) is
//               idle and waiting for us to send a request.
//
//               If persistent_connection is true, we will generally
//               come back to this state after finishing each request
//               on a given connection.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_ready() {
  // If there's a request to be sent upstream, send it now.
  if (!_request_text.empty()) {
   if (!http_send(_request_text)) {
      return true;
    }
  }
    
  // All done sending request.
  _state = S_request_sent;
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_request_sent
//       Access: Private
//  Description: In this state we have sent our request to the server
//               (or proxy) and we are waiting for a response.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_request_sent() {
  // Wait for the first line to come back from the server.
  string line;
  if (!http_getline(line)) {
    if (_bio.is_null()) {
      // Huh, the server hung up on us as soon as we tried to connect.
      if (_response_type == RT_hangup) {
        // This was our second immediate hangup in a row.  Give up.
        _state = S_failure;
        
      } else {
        // Try again, once.
        _response_type = RT_hangup;
      }
    }
    return true;
  }

  if (!parse_http_response(line)) {
    return false;
  }

  // Ok, we've established an HTTP connection to the server.  Our
  // extra send headers have done their job; clear them for next time.
  clear_extra_headers();

  _state = S_reading_header;
  _current_field_name = string();
  _current_field_value = string();
  _headers.clear();
  _file_size = 0;
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_reading_header
//       Access: Private
//  Description: In this state we have received the first response to
//               our request from the server (or proxy) and we are
//               reading the set of header lines preceding the
//               requested document.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_reading_header() {
  if (parse_http_header()) {
    return true;
  }

  _server_response_has_no_body = 
    (get_status_code() / 100 == 1 ||
     get_status_code() == 204 ||
     get_status_code() == 304 || 
     _method == HTTPEnum::M_head);

  // Look for key properties in the header fields.
  if (get_status_code() == 206) {
    string content_range = get_header_value("Content-Range");
    if (content_range.empty()) {
      downloader_cat.warning()
        << "Got 206 response without Content-Range header!\n";
      _state = S_failure;
      return false;

    } else {
      if (!parse_content_range(content_range)) {
        downloader_cat.warning()
          << "Couldn't parse Content-Range: " << content_range << "\n";
        _state = S_failure;
        return false;
      }
    }

  } else {
    _first_byte = 0;
    _last_byte = 0;
  }

  // In case we've got a download in effect, reset the download
  // position to match our starting byte.
  if (!reset_download_position()) {
    _state = S_failure;
    return false;
  }

  _file_size = 0;
  string content_length = get_header_value("Content-Length");
  if (!content_length.empty()) {
    _file_size = atoi(content_length.c_str());

  } else if (get_status_code() == 206) {
    // Well, we didn't get a content-length from the server, but we
    // can infer the number of bytes based on the range we're given.
    _file_size = _last_byte - _first_byte + 1;
  }
  _redirect = get_header_value("Location");

  _state = S_read_header;

  if (_server_response_has_no_body && will_close_connection()) {
    // If the server said it will close the connection, we should
    // close it too.
    close_connection();
  }

  // Handle automatic retries and redirects.
  int last_status = _last_status_code;
  _last_status_code = get_status_code();

  if (get_status_code() == 407 && last_status != 407 && !_proxy.empty()) {
    // 407: not authorized to proxy.  Try to get the authorization.
    string authenticate_request = get_header_value("Proxy-Authenticate");
    _proxy_auth = 
      _client->generate_auth(_proxy, true, authenticate_request);
    if (_proxy_auth != (HTTPAuthorization *)NULL) {
      _proxy_realm = _proxy_auth->get_realm();
      _proxy_username = _client->select_username(_proxy, true, _proxy_realm);
      if (!_proxy_username.empty()) {
        make_request_text();

        // Roll the state forward to force a new request.
        _state = S_begin_body;
        return false;
      }
    }
  }

  if (get_status_code() == 401 && last_status != 401) {
    // 401: not authorized to remote server.  Try to get the authorization.
    string authenticate_request = get_header_value("WWW-Authenticate");
    _www_auth = _client->generate_auth(_url, false, authenticate_request);
    if (_www_auth != (HTTPAuthorization *)NULL) {
      _www_realm = _www_auth->get_realm();
      _www_username = _client->select_username(_url, false, _www_realm);
      if (!_www_username.empty()) {
        make_request_text();
      
        // Roll the state forward to force a new request.
        _state = S_begin_body;
        return false;
      }
    }
  }

  if ((get_status_code() / 100) == 3 && get_status_code() != 305) {
    // Redirect.  Should we handle it automatically?
    if (!get_redirect().empty() && (_method == HTTPEnum::M_get || 
                                    _method == HTTPEnum::M_head)) {
      // Sure!
      URLSpec new_url = get_redirect();
      if (!_redirect_trail.insert(new_url).second) {
        downloader_cat.warning()
          << "cycle detected in redirect to " << new_url << "\n";
        
      } else {
        if (downloader_cat.is_debug()) {
          downloader_cat.debug()
            << "following redirect to " << new_url << "\n";
        }
        if (_url.has_username()) {
          new_url.set_username(_url.get_username());
        }
        set_url(new_url);
        make_header();
        make_request_text();

        // Roll the state forward to force a new request.
        _state = S_begin_body;
        return false;
      }
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_read_header
//       Access: Private
//  Description: In this state we have completely read the header
//               lines returned by the server (or proxy) in response
//               to our request.  This state represents the normal
//               stopping point of a call to get_document(), etc.;
//               further reads will return the body of the request,
//               the requested document.
//
//               Normally run_read_header() is not called unless the
//               user has elected not to read the returned document
//               himself.  In fact, the state itself only exists so we
//               can make a distinction between S_read_header and
//               S_begin_body, where S_read_header is safe to return
//               to the user and S_begin_body means we need to start
//               skipping the document.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_read_header() {
  _state = S_begin_body;
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_begin_body
//       Access: Private
//  Description: This state begins to skip over the body in
//               preparation for making a new request.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_begin_body() {
  if (will_close_connection()) {
    // If the socket will close anyway, no point in skipping past the
    // previous body; just reset.
    reset_to_new();
    return false;
  }

  if (_server_response_has_no_body) {
    // We have already "read" the nonexistent body.
    _state = S_read_trailer;

  } else if (_file_size > 8192) {
    // If we know the size of the body we are about to skip and it's
    // too large (and here we arbitrarily say 8KB is too large), then
    // don't bother skipping it--just drop the connection and get a
    // new one.
    if (downloader_cat.is_debug()) {
      downloader_cat.debug()
        << "Dropping connection rather than skipping past " << _file_size
        << " bytes.\n";
    }
    reset_to_new();

  } else {
    nassertr(_body_stream == NULL, false);
    _body_stream = read_body();
    if (_body_stream == (ISocketStream *)NULL) {
      if (downloader_cat.is_debug()) {
        downloader_cat.debug()
          << "Unable to skip body.\n";
      }
      reset_to_new();
      
    } else {
      _state = S_reading_body;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_reading_body
//       Access: Private
//  Description: In this state we are in the process of reading the
//               response's body.  We will only come to this function
//               if the user did not choose to read the entire body
//               himself (by calling read_body(), for instance, or
//               open_read_file()).
//
//               In this case we should skip past the body to reset
//               the connection for making a new request.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_reading_body() {
  if (will_close_connection()) {
    // If the socket will close anyway, no point in skipping past the
    // previous body; just reset.
    reset_to_new();
    return false;
  }

  // Skip the body we've already started.
  if (_body_stream == NULL) {
    // Whoops, we're not in skip-body mode.  Better reset.
    reset_to_new();
    return false;
  }

  string line;
  getline(*_body_stream, line);
  while (!_body_stream->fail() && !_body_stream->eof()) {
    if (downloader_cat.is_spam()) {
      downloader_cat.spam() << "skip: " << line << "\n";
    }
    getline(*_body_stream, line);
  }

  if (!_body_stream->is_closed()) {
    // There's more to come later.
    return true;
  }

  delete _body_stream;
  _body_stream = NULL;

  // This should have been set by the _body_stream finishing.
  nassertr(_state != S_reading_body, false);
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_read_body
//       Access: Private
//  Description: In this state we have completely read (or skipped
//               over) the body of the response.  We should continue
//               skipping past the trailer following the body.
//
//               Not all bodies come with trailers; in particular, the
//               "identity" transfer encoding does not include a
//               trailer.  It is therefore the responsibility of the
//               IdentityStreamBuf or ChunkedStreamBuf to set the
//               state appropriately to either S_read_body or
//               S_read_trailer following the completion of the body.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_read_body() {
  if (will_close_connection()) {
    // If the socket will close anyway, no point in skipping past the
    // previous body; just reset.
    reset_to_new();
    return false;
  }
  // Skip the trailer following the recently-read body.

  string line;
  if (!http_getline(line)) {
    return true;
  }
  while (!line.empty()) {
    if (!http_getline(line)) {
      return true;
    }
  }

  _state = S_read_trailer;
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_read_trailer
//       Access: Private
//  Description: In this state we have completely read the body and
//               the trailer.  This state is simply a pass-through
//               back to S_ready.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_read_trailer() {
  if (will_close_connection()) {
    // If the socket will close anyway, no point in skipping past the
    // previous body; just reset.
    reset_to_new();
    return false;
  }

  if (!_proxy.empty() && !_proxy_tunnel) {
    _state = S_proxy_ready;
  } else {
    _state = S_ready;
  }
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_download_to_file
//       Access: Private
//  Description: After the headers, etc. have been read, this streams
//               the download to the named file.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_download_to_file() {
  nassertr(_body_stream != (ISocketStream *)NULL, false);

  bool do_throttle = _nonblocking && _download_throttle;
  int count = 0;

  int ch = _body_stream->get();
  nassertr(_body_stream != (ISocketStream *)NULL, false);
  while (!_body_stream->eof() && !_body_stream->fail()) {
    _download_to_file.put(ch);
    _bytes_downloaded++;
    if (do_throttle && (++count >= _bytes_per_update)) {
      // That's enough for now.
      return true;
    }

    ch = _body_stream->get();
    nassertr(_body_stream != (ISocketStream *)NULL, false);
  }

  if (_download_to_file.fail()) {
    downloader_cat.warning()
      << "Error writing to " << _download_to_filename << "\n";
    _state = S_failure;
    _download_to_file.close();
    return false;
  }

  if (_body_stream->is_closed()) {
    // Done.
    _download_to_file.close();
    return false;
  } else {
    // More to come.
    return true;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::run_download_to_ram
//       Access: Private
//  Description: After the headers, etc. have been read, this streams
//               the download to the specified Ramfile object.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
run_download_to_ram() {
  nassertr(_body_stream != (ISocketStream *)NULL, false);
  nassertr(_download_to_ramfile != (Ramfile *)NULL, false);

  bool do_throttle = _nonblocking && _download_throttle;
  int count = 0;

  int ch = _body_stream->get();
  nassertr(_body_stream != (ISocketStream *)NULL, false);
  while (!_body_stream->eof() && !_body_stream->fail()) {
    _download_to_ramfile->_data += (char)ch;
    _bytes_downloaded++;
    if (do_throttle && (++count >= _bytes_per_update)) {
      // That's enough for now.
      return true;
    }

    ch = _body_stream->get();
    nassertr(_body_stream != (ISocketStream *)NULL, false);
  }

  if (_body_stream->is_closed()) {
    // Done.
    return false;
  } else {
    // More to come.
    return true;
  }
}


////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::begin_request
//       Access: Private
//  Description: Begins a new document request to the server, throwing
//               away whatever request was currently pending if
//               necessary.
////////////////////////////////////////////////////////////////////
void HTTPChannel::
begin_request(HTTPEnum::Method method, const URLSpec &url,
              const string &body, bool nonblocking, 
              size_t first_byte, size_t last_byte) {
  reset_for_new_request();

  // Changing the proxy, or the nonblocking state, is grounds for
  // dropping the old connection, if any.
  if (_proxy != _client->get_proxy()) {
    _proxy = _client->get_proxy();
    reset_to_new();
  }

  if (_nonblocking != nonblocking) {
    _nonblocking = nonblocking;
    reset_to_new();
  }

  set_url(url);
  _method = method;
  _body = body;

  // An https-style request means we'll need to establish an SSL
  // connection.
  _want_ssl = (_url.get_scheme() == "https");

  // If we have a proxy, we'll be asking the proxy to hand us the
  // document--except when we also have https, in which case we'll be
  // tunnelling through the proxy to talk to the server directly.
  _proxy_serves_document = (!_proxy.empty() && !_want_ssl);

  _first_byte = first_byte;
  _last_byte = last_byte;

  make_header();
  make_request_text();

  if (!_proxy.empty() && (_want_ssl || _method == HTTPEnum::M_connect)) {
    // Maybe we need to tunnel through the proxy to connect to the
    // server directly.  We need this for HTTPS, or if the user
    // requested a direct connection somewhere.
    ostringstream request;
    request 
      << "CONNECT " << _url.get_server_and_port()
      << " " << _client->get_http_version_string() << "\r\n";
    if (_client->get_http_version() >= HTTPEnum::HV_11) {
      request 
        << "Host: " << _url.get_server() << "\r\n";
    }
    _proxy_header = request.str();
    make_proxy_request_text();

  } else {
    _proxy_header = string();
    _proxy_request_text = string();
  }

  // Also, reset from whatever previous request might still be pending.
  if (_state == S_failure || (_state < S_read_header && _state != S_ready)) {
    reset_to_new();

  } else if (_state == S_read_header) {
    // Roll one step forwards to start skipping past the previous
    // body.
    _state = S_begin_body;
  }

  _done_state = (_method == HTTPEnum::M_connect) ? S_ready : S_read_header;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::reset_for_new_request
//       Access: Private
//  Description: Resets the internal state variables in preparation
//               for beginning a new request.
////////////////////////////////////////////////////////////////////
void HTTPChannel::
reset_for_new_request() {
  reset_download_to();
  _last_status_code = 0;
  _response_type = RT_none;
  _redirect_trail.clear();
  _bytes_downloaded = 0;
  _bytes_requested = 0;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::finished_body
//       Access: Private
//  Description: This is called by the body reading
//               classes--ChunkedStreamBuf and IdentityStreamBuf--when
//               they have finished reading the body.  It advances the
//               state appropriately.
//
//               has_trailer should be set true if the body type has
//               an associated trailer which should be read or
//               skipped, or false if there is no trailer.
////////////////////////////////////////////////////////////////////
void HTTPChannel::
finished_body(bool has_trailer) {
  if (will_close_connection() && _download_dest == DD_none) {
    reset_to_new();

  } else {
    if (has_trailer) {
      _state = HTTPChannel::S_read_body;
    } else {
      _state = HTTPChannel::S_read_trailer;
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::reset_download_position
//       Access: Private
//  Description: If a download has been requested, seeks within the
//               download file to the appropriate first_byte position,
//               so that downloaded bytes will be written to the
//               appropriate point within the file.  Returns true if
//               the starting position is valid, false otherwise.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
reset_download_position() {
  if (_subdocument_resumes) {
    if (_download_dest == DD_file) {
      // Windows doesn't complain if you try to seek past the end of
      // file--it happily appends enough zero bytes to make the
      // difference.  Blecch.  That means we need to get the file size
      // first to check it ourselves.
      _download_to_file.seekp(0, ios::end);
      if (_first_byte > (size_t)_download_to_file.tellp()) {
        downloader_cat.info()
          << "Invalid starting position of byte " << _first_byte << " within "
          << _download_to_filename << " (which has " 
          << _download_to_file.tellp() << " bytes)\n";
        _download_to_file.close();
        return false;
      }
      
      _download_to_file.seekp(_first_byte);
      
    } else if (_download_dest == DD_ram) {
      if (_first_byte > _download_to_ramfile->_data.length()) {
        downloader_cat.info()
          << "Invalid starting position of byte " << _first_byte 
          << " within Ramfile (which has " 
          << _download_to_ramfile->_data.length() << " bytes)\n";
        return false;
      }

      if (_first_byte == 0) {
        _download_to_ramfile->_data = string();
      } else {
        _download_to_ramfile->_data = 
          _download_to_ramfile->_data.substr(0, _first_byte);
      }
    }

  } else {
    // If _subdocument_resumes is false, we should be sure to reset to
    // the beginning of the file, regardless of the value of
    // _first_byte.
    if (_download_dest == DD_file) {
      _download_to_file.seekp(0);
    } else if (_download_dest == DD_ram) {
      _download_to_ramfile->_data = string();
    }
  }

  return true;
}


////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::http_getline
//       Access: Private
//  Description: Reads a single line from the server's reply.  Returns
//               true if the line is successfully retrieved, or false
//               if a complete line has not yet been received or if
//               the connection has been closed.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
http_getline(string &str) {
  nassertr(!_source.is_null(), false);
  int ch = (*_source)->get();
  while (!(*_source)->eof() && !(*_source)->fail()) {
    switch (ch) {
    case '\n':
      // end-of-line character, we're done.
      str = _working_getline;
      _working_getline = string();
      {
        // Trim trailing whitespace.  We're not required to do this per the
        // HTTP spec, but let's be generous.
        size_t p = str.length();
        while (p > 0 && isspace(str[p - 1])) {
          --p;
        }
        str = str.substr(0, p);
      }
      if (downloader_cat.is_spam()) {
        downloader_cat.spam() << "recv: " << str << "\n";
      }
      return true;

    case '\r':
      // Ignore CR characters.
      break;

    default:
      _working_getline += (char)ch;
    }
    ch = (*_source)->get();
  }

  check_socket();
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::http_send
//       Access: Private
//  Description: Sends a series of lines to the server.  Returns true
//               if the buffer is fully sent, or false if some of it
//               remains.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
http_send(const string &str) {
  nassertr(str.length() > _sent_so_far, true);

  // Use the underlying BIO to write to the server, instead of the
  // BIOStream, which would insist on blocking.
  size_t bytes_to_send = str.length() - _sent_so_far;
  int write_count =
    BIO_write(*_bio, str.data() + _sent_so_far, bytes_to_send);
    
  if (write_count <= 0) {
    if (BIO_should_retry(*_bio)) {
      // Temporary failure: the pipe is full.  Wait till later.
      return false;
    }
    // Oops, the connection has been closed!
    if (downloader_cat.is_debug()) {
      downloader_cat.debug()
        << "Lost connection to server unexpectedly during write.\n";
    }
    reset_to_new();
    return false;
  }
  
#ifndef NDEBUG
  if (downloader_cat.is_spam()) {
    show_send(str.substr(0, write_count));
  }
#endif
  
  if (write_count < (int)bytes_to_send) {
    _sent_so_far += write_count;
    return false;
  }

  // Buffer completely sent.
  _sent_so_far = 0;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::parse_http_response
//       Access: Private
//  Description: Parses the first line sent back from an HTTP server
//               or proxy and stores the result in _status_code and
//               _http_version, etc.  Returns true on success, false
//               on invalid response.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
parse_http_response(const string &line) {
  // The first line back should include the HTTP version and the
  // result code.
  if (line.length() < 5 || line.substr(0, 5) != "HTTP/") {
    // Not an HTTP response.
    _status_code = 0;
    _status_string = "Not an HTTP response";
    if (_response_type == RT_non_http) {
      // This was our second non-HTTP response in a row.  Give up.
      _state = S_failure;

    } else {
      // Maybe we were just in some bad state.  Drop the connection
      // and try again, once.
      reset_to_new();
      _response_type = RT_non_http;
    }
    return false;
  }

  // Okay, we're sane.
  _response_type = RT_http;

  // Split out the first line into its three components.
  size_t p = 5;
  while (p < line.length() && !isspace(line[p])) {
    p++;
  }
  _http_version_string = line.substr(0, p);
  _http_version = HTTPClient::parse_http_version_string(_http_version_string);

  while (p < line.length() && isspace(line[p])) {
    p++;
  }
  size_t q = p;
  while (q < line.length() && !isspace(line[q])) {
    q++;
  }
  string status_code = line.substr(p, q - p);
  _status_code = atoi(status_code.c_str());

  while (q < line.length() && isspace(line[q])) {
    q++;
  }
  _status_string = line.substr(q, line.length() - q);

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::parse_http_header
//       Access: Private
//  Description: Reads the series of header lines from the server and
//               stores them in _headers.  Returns true if there is
//               more to read, false when done.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
parse_http_header() {
  string line;
  if (!http_getline(line)) {
    return true;
  }

  while (!line.empty()) {
    if (isspace(line[0])) {
      // If the line begins with a space, that continues the previous
      // field.
      size_t p = 0;
      while (p < line.length() && isspace(line[p])) {
        p++;
      }
      _current_field_value += line.substr(p - 1);

    } else {
      // If the line does not begin with a space, that defines a new
      // field.
      if (!_current_field_name.empty()) {
        store_header_field(_current_field_name, _current_field_value);
        _current_field_value = string();
      }

      size_t colon = line.find(':');
      if (colon != string::npos) {
        _current_field_name = downcase(line.substr(0, colon));
        size_t p = colon + 1;
        while (p < line.length() && isspace(line[p])) {
          p++;
        }
        _current_field_value = line.substr(p);
      }
    }

    if (!http_getline(line)) {
      return true;
    }
  }

  // After reading an empty line, we're done with the headers.
  if (!_current_field_name.empty()) {
    store_header_field(_current_field_name, _current_field_value);
    _current_field_value = string();
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::parse_content_range
//       Access: Private
//  Description: Interprets the "Content-Range" header in the reply,
//               and fills in _first_byte and _last_byte appropriately
//               if the header response can be understood.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
parse_content_range(const string &content_range) {
  // First, get the units indication.
  size_t p = 0;
  while (p < content_range.length() && !isspace(content_range[p])) {
    p++;
  }

  string units = content_range.substr(0, p);
  while (p < content_range.length() && isspace(content_range[p])) {
    p++;
  }

  if (units == "bytes") {
    const char *c_str = content_range.c_str();
    char *endptr;
    if (p < content_range.length() && isdigit(content_range[p])) {
      long first_byte = strtol(c_str + p, &endptr, 10);
      p = endptr - c_str;
      if (p < content_range.length() && content_range[p] == '-') {
        p++;
        if (p < content_range.length() && isdigit(content_range[p])) {
          long last_byte = strtol(c_str + p, &endptr, 10);
          p = endptr - c_str;
          
          if (last_byte >= first_byte) {
            _first_byte = first_byte;
            _last_byte = last_byte;
            return true;
          }
        }
      }
    }
  }
    
  // Invalid or unhandled response.
  return false;
}


////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::check_socket
//       Access: Private
//  Description: Checks whether the connection to the server has been
//               closed after a failed read.  If it has, issues a
//               warning and calls reset_to_new().
////////////////////////////////////////////////////////////////////
void HTTPChannel::
check_socket() {
  nassertv(!_source.is_null());
  if ((*_source)->is_closed()) {
    if (downloader_cat.is_debug()) {
      downloader_cat.debug()
        << "Lost connection to server unexpectedly during read.\n";
    }
    reset_to_new();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::verify_server
//       Access: Private
//  Description: Returns true if the indicated server matches one of
//               our expected servers (or the list of expected servers
//               is empty), or false if it does not match any of our
//               expected servers.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
verify_server(X509_NAME *subject) const {
  if (_client->_expected_servers.empty()) {
    if (downloader_cat.is_debug()) {
      downloader_cat.debug()
        << "No expected servers on list; allowing any https connection.\n";
    }
    return true;
  }

  if (downloader_cat.is_debug()) {
    downloader_cat.debug() << "checking server: " << flush;
    X509_NAME_print_ex_fp(stderr, subject, 0, 0);
    fflush(stderr);
    downloader_cat.debug(false) << "\n";
  }

  HTTPClient::ExpectedServers::const_iterator ei;
  for (ei = _client->_expected_servers.begin();
       ei != _client->_expected_servers.end();
       ++ei) {
    X509_NAME *expected_name = (*ei);
    if (x509_name_subset(expected_name, subject)) {
      if (downloader_cat.is_debug()) {
        downloader_cat.debug()
          << "Match found!\n";
      }
      return true;
    }
  }

  // None of the lines matched.
  if (downloader_cat.is_debug()) {
    downloader_cat.debug()
      << "No match found against any of the following expected servers:\n";

    for (ei = _client->_expected_servers.begin();
         ei != _client->_expected_servers.end();
         ++ei) {
      X509_NAME *expected_name = (*ei);
      X509_NAME_print_ex_fp(stderr, expected_name, 0, 0);
      fprintf(stderr, "\n");
    }
    fflush(stderr);      
  }
  
  return false;
}

/*
   Certificate verify error codes:

0 X509_V_OK: ok

    the operation was successful.

2 X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT: unable to get issuer certificate

    the issuer certificate could not be found: this occurs if the
    issuer certificate of an untrusted certificate cannot be found.

3 X509_V_ERR_UNABLE_TO_GET_CRL unable to get certificate CRL

    the CRL of a certificate could not be found. Unused.

4 X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE: unable to decrypt
certificate's signature

    the certificate signature could not be decrypted. This means that
    the actual signature value could not be determined rather than it
    not matching the expected value, this is only meaningful for RSA
    keys.

5 X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE: unable to decrypt CRL's signature

    the CRL signature could not be decrypted: this means that the
    actual signature value could not be determined rather than it not
    matching the expected value. Unused.

6 X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY: unable to decode
issuer public key

    the public key in the certificate SubjectPublicKeyInfo could not
    be read.

7 X509_V_ERR_CERT_SIGNATURE_FAILURE: certificate signature failure

    the signature of the certificate is invalid.

8 X509_V_ERR_CRL_SIGNATURE_FAILURE: CRL signature failure

    the signature of the certificate is invalid. Unused.

9 X509_V_ERR_CERT_NOT_YET_VALID: certificate is not yet valid

    the certificate is not yet valid: the notBefore date is after the
    current time.

10 X509_V_ERR_CERT_HAS_EXPIRED: certificate has expired

    the certificate has expired: that is the notAfter date is before
    the current time.

11 X509_V_ERR_CRL_NOT_YET_VALID: CRL is not yet valid

    the CRL is not yet valid. Unused.

12 X509_V_ERR_CRL_HAS_EXPIRED: CRL has expired

    the CRL has expired. Unused.

13 X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD: format error in
certificate's notBefore field

    the certificate notBefore field contains an invalid time.

14 X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD: format error in
certificate's notAfter field

    the certificate notAfter field contains an invalid time.

15 X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD: format error in CRL's
lastUpdate field

    the CRL lastUpdate field contains an invalid time. Unused.

16 X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD: format error in CRL's
nextUpdate field

    the CRL nextUpdate field contains an invalid time. Unused.

17 X509_V_ERR_OUT_OF_MEM: out of memory

    an error occurred trying to allocate memory. This should never
    happen.

18 X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT: self signed certificate

    the passed certificate is self signed and the same certificate
    cannot be found in the list of trusted certificates.

19 X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN: self signed certificate in
certificate chain

    the certificate chain could be built up using the untrusted
    certificates but the root could not be found locally.

20 X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY: unable to get local
issuer certificate

    the issuer certificate of a locally looked up certificate could
    not be found. This normally means the list of trusted certificates
    is not complete.

21 X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE: unable to verify the
first certificate

    no signatures could be verified because the chain contains only
    one certificate and it is not self signed.

22 X509_V_ERR_CERT_CHAIN_TOO_LONG: certificate chain too long

    the certificate chain length is greater than the supplied maximum
    depth. Unused.

23 X509_V_ERR_CERT_REVOKED: certificate revoked

    the certificate has been revoked. Unused.

24 X509_V_ERR_INVALID_CA: invalid CA certificate

    a CA certificate is invalid. Either it is not a CA or its
    extensions are not consistent with the supplied purpose.

25 X509_V_ERR_PATH_LENGTH_EXCEEDED: path length constraint exceeded

    the basicConstraints pathlength parameter has been exceeded.

26 X509_V_ERR_INVALID_PURPOSE: unsupported certificate purpose

    the supplied certificate cannot be used for the specified purpose.

27 X509_V_ERR_CERT_UNTRUSTED: certificate not trusted

    the root CA is not marked as trusted for the specified purpose.

28 X509_V_ERR_CERT_REJECTED: certificate rejected

    the root CA is marked to reject the specified purpose.

29 X509_V_ERR_SUBJECT_ISSUER_MISMATCH: subject issuer mismatch

    the current candidate issuer certificate was rejected because its
    subject name did not match the issuer name of the current
    certificate. Only displayed when the -issuer_checks option is set.

30 X509_V_ERR_AKID_SKID_MISMATCH: authority and subject key identifier
mismatch

    the current candidate issuer certificate was rejected because its
    subject key identifier was present and did not match the authority
    key identifier current certificate. Only displayed when the
    -issuer_checks option is set.

31 X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH: authority and issuer serial
number mismatch

    the current candidate issuer certificate was rejected because its
    issuer name and serial number was present and did not match the
    authority key identifier of the current certificate. Only
    displayed when the -issuer_checks option is set.

32 X509_V_ERR_KEYUSAGE_NO_CERTSIGN:key usage does not include
certificate signing

    the current candidate issuer certificate was rejected because its
    keyUsage extension does not permit certificate signing.

50 X509_V_ERR_APPLICATION_VERIFICATION: application verification failure

    an application specific error. Unused.

*/

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::get_x509_name_component
//       Access: Private, Static
//  Description: Returns the indicated component of the X509 name as a
//               string, if defined, or empty string if it is not.
////////////////////////////////////////////////////////////////////
string HTTPChannel::
get_x509_name_component(X509_NAME *name, int nid) {
  ASN1_OBJECT *obj = OBJ_nid2obj(nid);

  if (obj == NULL) {
    // Unknown nid.  See openssl/objects.h.
    return string();
  }

  int i = X509_NAME_get_index_by_OBJ(name, obj, -1);
  if (i < 0) {
    return string();
  }

  ASN1_STRING *data = X509_NAME_ENTRY_get_data(X509_NAME_get_entry(name, i));
  return string((char *)data->data, data->length);  
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::x509_name_subset
//       Access: Private, Static
//  Description: Returns true if name_a is a subset of name_b: each
//               property of name_a is defined in name_b, and the
//               defined value is equivalent to that of name_a.
////////////////////////////////////////////////////////////////////
bool HTTPChannel::
x509_name_subset(X509_NAME *name_a, X509_NAME *name_b) {
  int count_a = X509_NAME_entry_count(name_a);
  for (int ai = 0; ai < count_a; ai++) {
    X509_NAME_ENTRY *na = X509_NAME_get_entry(name_a, ai);

    int bi = X509_NAME_get_index_by_OBJ(name_b, na->object, -1);
    if (bi < 0) {
      // This entry in name_a is not defined in name_b.
      return false;
    }

    X509_NAME_ENTRY *nb = X509_NAME_get_entry(name_b, bi);
    if (na->value->length != nb->value->length ||
        memcmp(na->value->data, nb->value->data, na->value->length) != 0) {
      // This entry in name_a doesn't match that of name_b.
      return false;
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::make_header
//       Access: Private
//  Description: Formats the appropriate GET or POST (or whatever)
//               request to send to the server, based on the current
//               _method, _url, _body, and _proxy settings.
////////////////////////////////////////////////////////////////////
void HTTPChannel::
make_header() {
  _proxy_auth = _client->select_auth(_proxy, true, _proxy_realm);
  _proxy_username = string();
  if (_proxy_auth != (HTTPAuthorization *)NULL) {
    _proxy_realm = _proxy_auth->get_realm();
    _proxy_username = _client->select_username(_proxy, true, _proxy_realm);
  }

  if (_method == HTTPEnum::M_connect) {
    // This method doesn't require an HTTP header at all; we'll just
    // open a plain connection.  (Except when we're using a proxy; but
    // in that case, it's the proxy_header we'll need, not the regular
    // HTTP header.)
    _header = string();
    return;
  }

  _www_auth = _client->select_auth(_url, false, _www_realm);
  _www_username = string();
  if (_www_auth != (HTTPAuthorization *)NULL) {
    _www_realm = _www_auth->get_realm();
    _www_username = _client->select_username(_url, false, _www_realm);
  }

  string request_path;
  if (_proxy_serves_document) {
    // If we'll be asking the proxy for the document, we need its full
    // URL--but we omit the username, which is information just for us.
    URLSpec url_no_username = _url;
    url_no_username.set_username(string());
    request_path = url_no_username.get_url();

  } else {
    // If we'll be asking the server directly for the document, we
    // just want its path relative to the server.
    request_path = _url.get_path();
  }

  ostringstream stream;

  stream 
    << _method << " " << request_path << " " 
    << _client->get_http_version_string() << "\r\n";

  if (_client->get_http_version() >= HTTPEnum::HV_11) {
    stream 
      << "Host: " << _url.get_server() << "\r\n";
    if (!get_persistent_connection()) {
      stream
        << "Connection: close\r\n";
    }
  }

  if (_last_byte != 0) {
    stream 
      << "Range: bytes=" << _first_byte << "-" << _last_byte << "\r\n";

  } else if (_first_byte != 0) {
    stream 
      << "Range: bytes=" << _first_byte << "-\r\n";
  }

  if (!_body.empty()) {
    stream
      << "Content-Type: application/x-www-form-urlencoded\r\n"
      << "Content-Length: " << _body.length() << "\r\n";
  }

  _header = stream.str();
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::make_proxy_request_text
//       Access: Private
//  Description: Builds the _proxy_request_text string.  This is a
//               special request that will be sent directly to the
//               proxy prior to the request tailored for the server.
//               Generally this is used to open a tunnelling
//               connection for https-over-proxy.
////////////////////////////////////////////////////////////////////
void HTTPChannel::
make_proxy_request_text() {
  _proxy_request_text = _proxy_header;

  if (_proxy_auth != (HTTPAuthorization *)NULL && !_proxy_username.empty()) {
    _proxy_request_text += "Proxy-Authorization: ";
    _proxy_request_text += 
      _proxy_auth->generate(HTTPEnum::M_connect, _url.get_server_and_port(),
                            _proxy_username, _body);
    _proxy_request_text += "\r\n";
  }
    
  _proxy_request_text += "\r\n";
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::make_request_text
//       Access: Private
//  Description: Builds the _request_text string.  This is the
//               specific request that will be sent to the server this
//               pass, based on the current header and body.
////////////////////////////////////////////////////////////////////
void HTTPChannel::
make_request_text() {
  _request_text = _header;

  if (!_proxy.empty() && !_proxy_tunnel &&
      _proxy_auth != (HTTPAuthorization *)NULL && !_proxy_username.empty()) {
    _request_text += "Proxy-Authorization: ";
    _request_text += 
      _proxy_auth->generate(_method, _url.get_url(), _proxy_username, _body);
    _request_text += "\r\n";
  }

  if (_www_auth != (HTTPAuthorization *)NULL && !_www_username.empty()) {
    string authorization = 
    _request_text += "Authorization: ";
    _request_text +=
      _www_auth->generate(_method, _url.get_path(), _www_username, _body);
    _request_text += "\r\n";
  }

  _request_text += _send_extra_headers;
  _request_text += "\r\n";
  _request_text += _body;
}
  
////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::set_url
//       Access: Private
//  Description: Specifies the document's URL before attempting a
//               connection.  This controls the name of the server to
//               be contacted, etc.
////////////////////////////////////////////////////////////////////
void HTTPChannel::
set_url(const URLSpec &url) {
  // If we change between http and https, we have to reset the
  // connection regardless of proxy.  Otherwise, we have to drop the
  // connection if the server or port changes, unless we're
  // communicating through a proxy.

  if (url.get_scheme() != _url.get_scheme() ||
      (_proxy.empty() && (url.get_server() != _url.get_server() || 
                          url.get_port() != _url.get_port()))) {
    reset_to_new();
  }
  _url = url;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::store_header_field
//       Access: Private
//  Description: Stores a single name: value pair in the header list,
//               or appends the value to the end of the existing
//               value, if the header has been repeated.
////////////////////////////////////////////////////////////////////
void HTTPChannel::
store_header_field(const string &field_name, const string &field_value) {
  pair<Headers::iterator, bool> insert_result =
    _headers.insert(Headers::value_type(field_name, field_value));

  if (!insert_result.second) {
    // It didn't insert; thus, the field already existed.  Append the
    // new value.
    Headers::iterator hi = insert_result.first;
    (*hi).second += ", ";
    (*hi).second += field_value;
  }
}

#ifndef NDEBUG
////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::show_send
//       Access: Private, Static
//  Description: Writes the outgoing message, one line at a time, to
//               the debugging log.
////////////////////////////////////////////////////////////////////
void HTTPChannel::
show_send(const string &message) {
  size_t start = 0;
  size_t newline = message.find('\n', start);
  while (newline != string::npos) {
    // Assume every \n is preceded by a \r.
    downloader_cat.spam()
      << "send: " << message.substr(start, newline - start - 1) << "\n";
    start = newline + 1;
    newline = message.find('\n', start);
  }

  if (start < message.length()) {
    downloader_cat.spam()
      << "send: " << message.substr(start) << " (no newline)\n";
  }
}
#endif   // NDEBUG

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::reset_download_to
//       Access: Private
//  Description: Resets the indication of how the document will be
//               downloaded.  This must be re-specified after each
//               get_document() (or related) call.
////////////////////////////////////////////////////////////////////
void HTTPChannel::
reset_download_to() {
  _started_download = false;
  _download_to_file.close();
  _download_to_ramfile = (Ramfile *)NULL;
  _download_dest = DD_none;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::reset_to_new
//       Access: Private
//  Description: Closes the connection and resets the state to S_new.
////////////////////////////////////////////////////////////////////
void HTTPChannel::
reset_to_new() {
  close_connection();
  _state = S_new;
}

////////////////////////////////////////////////////////////////////
//     Function: HTTPChannel::close_connection
//       Access: Private
//  Description: Closes the connection but leaves the _state
//               unchanged.
////////////////////////////////////////////////////////////////////
void HTTPChannel::
close_connection() {
  if (_body_stream != (ISocketStream *)NULL) {
    delete _body_stream;
    _body_stream = (ISocketStream *)NULL;
  }
  _source.clear();
  _bio.clear();
  _working_getline = string();
  _sent_so_far = 0;
  _proxy_tunnel = false;
  _read_index++;
}

#endif  // HAVE_SSL
