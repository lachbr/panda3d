// Filename: httpChannel.h
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

#ifndef HTTPCHANNEL_H
#define HTTPCHANNEL_H

#include "pandabase.h"

// This module requires OpenSSL to compile, even if you do not intend
// to use this to establish https connections; this is because it uses
// the OpenSSL library to portably handle all of the socket
// communications.

#ifdef HAVE_SSL

#include "httpClient.h"
#include "httpEnum.h"
#include "urlSpec.h"
#include "virtualFile.h"
#include "bioPtr.h"
#include "bioStreamPtr.h"
#include "pmap.h"
#include "pointerTo.h"
#include "config_downloader.h"
#include "filename.h"
#include <openssl/ssl.h>

class Ramfile;
class HTTPClient;

////////////////////////////////////////////////////////////////////
//       Class : HTTPChannel
// Description : A single channel of communication from an HTTPClient.
//               This is similar to the concept of a 'connection',
//               except that HTTP is technically connectionless; in
//               fact, a channel may represent one unbroken connection
//               or it may transparently close and reopen a new
//               connection with each request.
//
//               A channel is conceptually a single thread of I/O.
//               One document at a time may be requested using a
//               channel; a new document may (in general) not be
//               requested from the same HTTPChannel until the first
//               document has been fully retrieved.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEXPRESS HTTPChannel : public VirtualFile {
private:
  HTTPChannel(HTTPClient *client);

public:
  virtual ~HTTPChannel();

  virtual VirtualFileSystem *get_file_system() const;
  virtual Filename get_filename() const;

  virtual bool is_regular_file() const;
  virtual istream *open_read_file() const;

  bool will_close_connection() const;

PUBLISHED:
  INLINE bool is_valid() const;
  INLINE bool is_connection_ready() const;
  INLINE const URLSpec &get_url() const;
  INLINE HTTPEnum::HTTPVersion get_http_version() const;
  INLINE const string &get_http_version_string() const;
  INLINE int get_status_code() const;
  INLINE const string &get_status_string() const;
  INLINE const string &get_www_realm() const;
  INLINE const string &get_proxy_realm() const;
  INLINE const URLSpec &get_redirect() const;
  string get_header_value(const string &key) const;

  INLINE void set_persistent_connection(bool persistent_connection);
  INLINE bool get_persistent_connection() const;

  INLINE void set_connect_timeout(double timeout_seconds);
  INLINE double get_connect_timeout() const;
  INLINE void set_blocking_connect(bool blocking_connect);
  INLINE bool get_blocking_connect() const;

  INLINE void set_download_throttle(bool download_throttle);
  INLINE bool get_download_throttle() const;

  INLINE void set_max_bytes_per_second(double max_bytes_per_second);
  INLINE double get_max_bytes_per_second() const;

  INLINE void set_max_updates_per_second(double max_updates_per_second);
  INLINE double get_max_updates_per_second() const;

  INLINE size_t get_file_size() const;

  void write_headers(ostream &out) const;

  INLINE void reset();

  INLINE bool get_document(const URLSpec &url);
  INLINE bool get_subdocument(const URLSpec &url, 
                              size_t first_byte, size_t last_byte);
  INLINE bool get_header(const URLSpec &url);
  INLINE bool post_form(const URLSpec &url, const string &body);
  INLINE bool put_document(const URLSpec &url, const string &body);
  INLINE bool delete_document(const URLSpec &url);
  INLINE bool get_trace(const URLSpec &url);
  INLINE bool connect_to(const URLSpec &url);

  INLINE void begin_get_document(const URLSpec &url);
  INLINE void begin_get_subdocument(const URLSpec &url, 
                                    size_t first_byte, size_t last_byte);
  INLINE void begin_get_header(const URLSpec &url);
  INLINE void begin_post_form(const URLSpec &url, const string &body);
  bool run();
  INLINE void begin_connect_to(const URLSpec &url);

  ISocketStream *read_body();
  bool download_to_file(const Filename &filename, size_t first_byte = 0);
  bool download_to_ram(Ramfile *ramfile);
  SocketStream *get_connection();

  INLINE size_t get_bytes_downloaded() const;
  INLINE size_t get_bytes_requested() const;
  INLINE bool is_download_complete() const;

public:
  static string downcase(const string &s);

private:
  bool reached_done_state();
  bool run_connecting();
  bool run_connecting_wait();
  bool run_proxy_ready();
  bool run_proxy_request_sent();
  bool run_proxy_reading_header();
  bool run_setup_ssl();
  bool run_ssl_handshake();
  bool run_ready();
  bool run_request_sent();
  bool run_reading_header();
  bool run_read_header();
  bool run_begin_body();
  bool run_reading_body();
  bool run_read_body();
  bool run_read_trailer();

  bool run_download_to_file();
  bool run_download_to_ram();

  void begin_request(HTTPEnum::Method method, const URLSpec &url, 
                     const string &body, bool nonblocking,
                     size_t first_byte, size_t last_byte);
  void reset_for_new_request();

  void finished_body(bool has_trailer);

  bool http_getline(string &str);
  bool http_send(const string &str);
  bool parse_http_response(const string &line);
  bool parse_http_header();
  bool parse_content_range(const string &content_range);

  void check_socket();
  bool verify_server(X509_NAME *subject) const;

  static string get_x509_name_component(X509_NAME *name, int nid);
  static bool x509_name_subset(X509_NAME *name_a, X509_NAME *name_b);

  void make_header();
  void make_proxy_request_text();
  void make_request_text();

  void set_url(const URLSpec &url);
  void store_header_field(const string &field_name, const string &field_value);

#ifndef NDEBUG
  static void show_send(const string &message);
#endif

  void reset_download_to();
  void reset_to_new();
  void close_connection();

  HTTPClient *_client;
  URLSpec _proxy;
  PT(BioPtr) _bio;
  PT(BioStreamPtr) _source;
  bool _persistent_connection;
  double _connect_timeout;
  bool _blocking_connect;
  bool _download_throttle;
  double _max_bytes_per_second;
  double _max_updates_per_second;
  double _seconds_per_update;
  int _bytes_per_update;
  bool _nonblocking;

  URLSpec _url;
  HTTPEnum::Method _method;
  string request_path;
  string _header;
  string _body;
  bool _want_ssl;
  bool _proxy_serves_document;
  bool _server_response_has_no_body;
  size_t _first_byte;
  size_t _last_byte;

  enum DownloadDest {
    DD_none,
    DD_file,
    DD_ram,
  };
  DownloadDest _download_dest;
  Filename _download_to_filename;
  ofstream _download_to_file;
  Ramfile *_download_to_ramfile;

  int _read_index;

  HTTPEnum::HTTPVersion _http_version;
  string _http_version_string;
  int _status_code;
  string _status_string;
  URLSpec _redirect;

  string _proxy_realm;
  string _proxy_username;
  PT(HTTPAuthorization) _proxy_auth;

  string _www_realm;
  string _www_username;
  PT(HTTPAuthorization) _www_auth;

  enum ResponseType {
    RT_none,
    RT_hangup,
    RT_non_http,
    RT_http
  };
  ResponseType _response_type;
  

  typedef pmap<string, string> Headers;
  Headers _headers;

  size_t _file_size;
  size_t _bytes_downloaded;
  size_t _bytes_requested;

  // These members are used to maintain the current state while
  // communicating with the server.  We need to store everything in
  // the class object instead of using local variables because in the
  // case of nonblocking I/O we have to be able to return to the
  // caller after any I/O operation and resume later where we left
  // off.
  enum State {
    S_new,
    S_connecting,
    S_connecting_wait,
    S_proxy_ready,
    S_proxy_request_sent,
    S_proxy_reading_header,
    S_setup_ssl,
    S_ssl_handshake,
    S_ready,
    S_request_sent,
    S_reading_header,
    S_read_header,
    S_begin_body,
    S_reading_body,
    S_read_body,
    S_read_trailer,
    S_failure
  };
  State _state;
  State _done_state;
  double _started_connecting_time;
  bool _started_download;
  string _proxy_header;
  string _proxy_request_text;
  bool _proxy_tunnel;
  string _request_text;
  string _working_getline;
  size_t _sent_so_far;
  string _current_field_name;
  string _current_field_value;
  ISocketStream *_body_stream;
  BIO *_sbio;
  pset<URLSpec> _redirect_trail;
  int _last_status_code;
  double _last_run_time;

public:
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    VirtualFile::init_type();
    register_type(_type_handle, "HTTPChannel",
                  VirtualFile::get_class_type());
  }

private:
  static TypeHandle _type_handle;
  friend class ChunkedStreamBuf;
  friend class IdentityStreamBuf;
  friend class HTTPClient;
};

#include "httpChannel.I"

#endif  // HAVE_SSL

#endif


