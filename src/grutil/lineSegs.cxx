// Filename: lineSegs.cxx
// Created by:   (24May00)
// 
////////////////////////////////////////////////////////////////////

#include "lineSegs.h"

////////////////////////////////////////////////////////////////////
//     Function: LineSegs::Constructor
//       Access: Public
//  Description: Constructs a LineSegs object, which can be used to
//               create any number of disconnected lines or points of
//               various thicknesses and colors through the visible
//               scene.  After creating the object, call move_to() and
//               draw_to() repeatedly to describe the path, then call
//               create() to create a GeomNode which will render the
//               described path.
////////////////////////////////////////////////////////////////////
LineSegs::
LineSegs() {
  _color.set(1.0, 1.0, 1.0, 1.0);
  _thick = 1.0;
}


////////////////////////////////////////////////////////////////////
//     Function: LineSegs::Destructor
//       Access: Public
////////////////////////////////////////////////////////////////////
LineSegs::
~LineSegs() {
}


////////////////////////////////////////////////////////////////////
//     Function: LineSegs::reset
//       Access: Public
//  Description: Removes any lines in progress and resets to the
//               initial empty state.
////////////////////////////////////////////////////////////////////
void LineSegs::
reset() {
  _list.clear();
}


////////////////////////////////////////////////////////////////////
//     Function: LineSegs::move_to
//       Access: Public
//  Description: Moves the pen to the given point without drawing a
//               line.  When followed by draw_to(), this marks the
//               first point of a line segment; when followed by
//               move_to() or create(), this creates a single point.
////////////////////////////////////////////////////////////////////
void LineSegs::
move_to(const Vertexf &v) {
  // We create a new SegmentList with the initial point in it.
  SegmentList segs;
  segs.push_back(Point(v, _color));

  // And add this list to the list of segments.
  _list.push_back(segs);
}

////////////////////////////////////////////////////////////////////
//     Function: LineSegs::draw_to
//       Access: Public
//  Description: Draws a line segment from the pen's last position
//               (the last call to move_to or draw_to) to the
//               indicated point.  move_to() and draw_to() only update
//               tables; the actual drawing is performed when create()
//               is called.
////////////////////////////////////////////////////////////////////
void LineSegs::
draw_to(const Vertexf &v) {
  if (_list.empty()) {
    // Let our first call to draw_to() be an implicit move_to().
    move_to(v);

  } else {
    // Get the current SegmentList, which was the last one we added to
    // the LineList.
    SegmentList &segs = _list.back();
    
    // Add the new point.
    segs.push_back(Point(v, _color));
  }
}

////////////////////////////////////////////////////////////////////
//     Function: LineSegs::empty
//       Access: Public
//  Description: Returns true if move_to() or draw_to() have not been
//               called since the last reset() or create(), false
//               otherwise.
////////////////////////////////////////////////////////////////////
bool LineSegs::
is_empty() {
  return _list.empty();
}  

////////////////////////////////////////////////////////////////////
//     Function: LineSegs::get_current_position
//       Access: Public
//  Description: Returns the pen's current position.  The next call to
//               draw_to() will draw a line segment from this point.
////////////////////////////////////////////////////////////////////
const Vertexf &LineSegs::
get_current_position() {
  if (_list.empty()) {
    // Our pen isn't anywhere.  We'll put it somewhere.
    move_to(Vertexf(0.0, 0.0, 0.0));
  }

  return _list.back().back()._point;
}

////////////////////////////////////////////////////////////////////
//     Function: LineSegs::create
//       Access: Public
//  Description: Appends to an existing GeomNode a new pfGeoSet that
//               will render the series of line segments and points
//               described via calls to move_to() and draw_to().  The
//               lines and points are created with the color and
//               thickness established by calls to set_color() and
//               set_thick().
//
//               If frame_accurate is true, the line segments will be
//               created as a frame-accurate index, so that later
//               calls to set_vertex or set_vertex_color will be
//               visually correct.
////////////////////////////////////////////////////////////////////
GeomNode *LineSegs::
create(GeomNode *previous, bool) {
  if (!_list.empty()) {
    _created_verts.clear();
    _created_colors.clear();

    // One array each for the indices into these arrays for points
    // and lines, and one for our line-segment lengths array.
    PTA_ushort point_index;
    PTA_ushort line_index;
    PTA_int lengths;

    // Now fill up the arrays.
    int v = 0;
    LineList::const_iterator ll;
    SegmentList::const_iterator sl;

    for (ll = _list.begin(); ll != _list.end(); ll++) {
      const SegmentList &segs = (*ll);

      if (segs.size() < 2) {
	point_index.push_back(v);
      } else {
	lengths.push_back(segs.size());
      }
      
      for (sl = segs.begin(); sl != segs.end(); sl++) {
	if (segs.size() >= 2) {
	  line_index.push_back(v);
	}
	_created_verts.push_back((*sl)._point);
	_created_colors.push_back((*sl)._color);
	v++;
	nassertr(v == _created_verts.size(), previous);
      }
    }
      
    // Now create the lines.
    if (line_index.size() > 0) {
      // Create a new Geom and add the line segments.
      Geom *geom;
      if (line_index.size() <= 2) {
	// Here's a special case: just one line segment.
	GeomLine *geom_line = new GeomLine;
	geom_line->set_num_prims(1);
	geom_line->set_width(_thick);
	geom = geom_line;
	
      } else {
	// The more normal case: multiple line segments, connected
	// end-to-end like a series of linestrips.
	GeomLinestrip *geom_linestrip = new GeomLinestrip;
	geom_linestrip->set_num_prims(lengths.size());
	geom_linestrip->set_lengths(lengths);
	geom_linestrip->set_width(_thick);
	geom = geom_linestrip;
      }

      geom->set_colors(_created_colors, G_PER_VERTEX, line_index);
      geom->set_coords(_created_verts, G_PER_VERTEX, line_index);

      previous->add_geom(geom);
    }
    
    // And now create the points.
    if (point_index.size() > 0) {
      // Create a new Geom and add the points.
      GeomPoint *geom = new GeomPoint;
	
      geom->set_num_prims(point_index.size());
      geom->set_size(_thick);
      geom->set_colors(_created_colors, G_PER_VERTEX, point_index);
      geom->set_coords(_created_verts, G_PER_VERTEX, point_index);
	
      previous->add_geom(geom);
    }

    // And reset for next time.
    reset();
  }

  return previous;
}
