// Filename: httpAuthorization.h
// Created by:  drose (22Oct02)
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

#ifndef HTTPAUTHORIZATION_H
#define HTTPAUTHORIZATION_H

#include "pandabase.h"

// This module requires OpenSSL to compile, even though it doesn't
// actually use any OpenSSL code, because it is a support module for
// HTTPChannel, which *does* use OpenSSL code.

#ifdef HAVE_SSL

#include "referenceCount.h"
#include "httpEnum.h"

class URLSpec;

////////////////////////////////////////////////////////////////////
//       Class : HTTPAuthorization
// Description : A base class for storing information used to fulfill
//               authorization requests in the past, which can
//               possibly be re-used for future requests to the same
//               server.
//
//               This class does not need to be exported from the DLL
//               because it has no public interface; it is simply a
//               helper class for HTTPChannel.
////////////////////////////////////////////////////////////////////
class HTTPAuthorization : public ReferenceCount {
public:
  typedef pmap<string, string> Tokens;
  typedef pmap<string, Tokens> AuthenticationSchemes;

protected:
  HTTPAuthorization(const Tokens &tokens, const URLSpec &url,
                    bool is_proxy);
public:
  virtual ~HTTPAuthorization();

  virtual const string &get_mechanism() const=0;
  virtual bool is_valid();

  INLINE const string &get_realm() const;
  INLINE const vector_string &get_domain() const;

  virtual string generate(HTTPEnum::Method method, const string &request_path,
                          const string &username, const string &body)=0;

  static void parse_authentication_schemes(AuthenticationSchemes &schemes,
                                           const string &field_value);
  static URLSpec get_canonical_url(const URLSpec &url);
  static string base64_encode(const string &s);

protected:
  static size_t scan_quoted_or_unquoted_string(string &result, 
                                               const string &source, 
                                               size_t start);

protected:
  string _realm;
  vector_string _domain;
};

#include "httpAuthorization.I"

#endif  // HAVE_SSL

#endif

