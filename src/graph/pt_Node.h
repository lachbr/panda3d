// Filename: pt_Node.h
// Created by:  drose (16May00)
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

#ifndef PT_NODE_H
#define PT_NODE_H

#include <pandabase.h>

// We can't include node.h, because we need to include this in
// nodeRelation.h.

#include <pointerTo.h>

class Node;

////////////////////////////////////////////////////////////////////
//       Class : PT_Node
// Description : A PT(Node).  This is defined here solely we can
//               explicitly export the template class.  It's not
//               strictly necessary, but it doesn't hurt.
////////////////////////////////////////////////////////////////////

EXPORT_TEMPLATE_CLASS(EXPCL_PANDA, EXPTP_PANDA, PointerToBase<Node>)
EXPORT_TEMPLATE_CLASS(EXPCL_PANDA, EXPTP_PANDA, PointerTo<Node>)
EXPORT_TEMPLATE_CLASS(EXPCL_PANDA, EXPTP_PANDA, ConstPointerTo<Node>)

typedef PointerTo<Node> PT_Node;
typedef ConstPointerTo<Node> CPT_Node;

// Tell GCC that we'll take care of the instantiation explicitly here.
#ifdef __GNUC__
#pragma interface
#endif

#endif
