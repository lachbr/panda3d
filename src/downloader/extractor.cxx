// Filename: extractor.cxx
// Created by:  mike (09Jan97)
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

#include "extractor.h"
#include "config_downloader.h"

#include "filename.h"
#include "error_utils.h"


////////////////////////////////////////////////////////////////////
//     Function: Extractor::Constructor
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
Extractor::
Extractor() {
  _initiated = false;
}

////////////////////////////////////////////////////////////////////
//     Function: Extractor::Destructor
//       Access: Published
//  Description:
////////////////////////////////////////////////////////////////////
Extractor::
~Extractor() {
  reset();
}

////////////////////////////////////////////////////////////////////
//     Function: Extractor::set_multifile
//       Access: Published
//  Description: Specifies the filename of the Multifile that the
//               Extractor will read.  Returns true on success, false
//               if the mulifile name is invalid.
////////////////////////////////////////////////////////////////////
bool Extractor::
set_multifile(const Filename &multifile_name) {
  reset();
  _multifile_name = multifile_name;
  return _multifile.open_read(multifile_name);
}

////////////////////////////////////////////////////////////////////
//     Function: Extractor::set_extract_dir
//       Access: Published
//  Description: Specifies the directory into which all extracted
//               subfiles will be written.  Relative paths of subfiles
//               within the Multifile will be written as relative
//               paths to this directory.
////////////////////////////////////////////////////////////////////
void Extractor::
set_extract_dir(const Filename &extract_dir) {
  _extract_dir = extract_dir;
}

////////////////////////////////////////////////////////////////////
//     Function: Extractor::reset
//       Access: Published
//  Description: Interrupts the Extractor in the middle of its
//               business and makes it ready to accept a new list of
//               subfiles to extract.
////////////////////////////////////////////////////////////////////
void Extractor::
reset() {
  if (_initiated) {
    if (_read != (istream *)NULL) {
      delete _read;
      _read = (istream *)NULL;
    }
    _write.close();
    _initiated = false;
  }

  _requests.clear();
}

////////////////////////////////////////////////////////////////////
//     Function: Extractor::request_subfile
//       Access: Published
//  Description: Requests a particular subfile to be extracted when
//               step() or run() is called.  Returns true if the
//               subfile exists, false otherwise.
////////////////////////////////////////////////////////////////////
bool Extractor::
request_subfile(const Filename &subfile_name) {
  int index = _multifile.find_subfile(subfile_name);
  if (index < 0) {
    return false;
  }
  _requests.push_back(index);
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: Extractor::request_all_subfiles
//       Access: Published
//  Description: Requests all subfiles in the Multifile to be
//               extracted.  Returns the number requested.
////////////////////////////////////////////////////////////////////
int Extractor::
request_all_subfiles() {
  _requests.clear();
  int num_subfiles = _multifile.get_num_subfiles();
  for (int i = 0; i < num_subfiles; i++) {
    _requests.push_back(i);
  }
  return num_subfiles;
}

////////////////////////////////////////////////////////////////////
//     Function: Extractor::step
//       Access: Published
//  Description: After all of the requests have been made via
//               request_file() or request_all_subfiles(), call step()
//               repeatedly until it stops returning EU_ok.
//
//               step() extracts the next small unit of data from the
//               Multifile.  Returns EU_ok if progress is continuing,
//               EU_error_abort if there is a problem, or EU_success
//               when the last piece has been extracted.
//
//               Also see run().
////////////////////////////////////////////////////////////////////
int Extractor::
step() {
  if (!_initiated) {
    _request_index = 0;
    _subfile_index = 0;
    _subfile_pos = 0;
    _subfile_length = 0;
    _read = (istream *)NULL;
    _initiated = true;
  }

  if (_read == (istream *)NULL) {
    // Time to open the next subfile.
    if (_request_index >= (int)_requests.size()) {
      // All done!
      reset();
      return EU_success;
    }

    _subfile_index = _requests[_request_index];
    Filename subfile_filename(_extract_dir, 
                              _multifile.get_subfile_name(_subfile_index));
    subfile_filename.set_binary();
    subfile_filename.make_dir();
    if (!subfile_filename.open_write(_write)) {
      downloader_cat.error()
        << "Unable to write to " << subfile_filename << ".\n";
      reset();
      return EU_error_abort;
    }

    _subfile_length = _multifile.get_subfile_length(_subfile_index);
    _subfile_pos = 0;
    _read = _multifile.open_read_subfile(_subfile_index);
    if (_read == (istream *)NULL) {
      downloader_cat.error()
        << "Unable to read subfile "
        << _multifile.get_subfile_name(_subfile_index) << ".\n";
      cleanup();
      return EU_error_abort;
    }

  } else if (_subfile_pos >= _subfile_length) {
    // Time to close this subfile.
    delete _read;
    _read = (istream *)NULL;
    _write.close();
    _request_index++;

  } else {
    // Read a number of bytes from the subfile and write them to the
    // output.
    size_t max_bytes = min((size_t)extractor_buffer_size, 
                           _subfile_length - _subfile_pos);
    for (size_t p = 0; p < max_bytes; p++) {
      int byte = _read->get();
      if (_read->eof() || _read->fail()) {
        downloader_cat.error()
          << "Unexpected EOF on multifile " << _multifile_name << ".\n";
        reset();
        return EU_error_abort;
      }
      _write.put(byte);
    }
    _subfile_pos += max_bytes;
  }

  return EU_ok;
}

////////////////////////////////////////////////////////////////////
//     Function: Extractor::run
//       Access: Published
//  Description: A convenience function to extract the Multifile all
//               at once, when you don't care about doing it in the
//               background.
//
//               First, call request_file() or request_all_files() to
//               specify the files you would like to extract, then
//               call run() to do the extraction.  Also see step() for
//               when you would like the extraction to happen as a
//               background task.
////////////////////////////////////////////////////////////////////
bool Extractor::
run() {
  while (true) {
    int ret = step();
    if (ret == EU_success) {
      return true;
    }
    if (ret < 0) {
      return false;
    }
  }
  return false;
}
