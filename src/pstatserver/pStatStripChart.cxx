// Filename: pStatStripChart.cxx
// Created by:  drose (15Jul00)
// 
////////////////////////////////////////////////////////////////////

#include "pStatStripChart.h"

#include <pStatFrameData.h>
#include <pStatCollectorDef.h>
#include <string_utils.h>
#include <config_pstats.h>

#include <stdio.h>  // for sprintf
#include <algorithm>

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
PStatStripChart::
PStatStripChart(PStatMonitor *monitor, PStatView &view,
		int collector_index, int xsize, int ysize) :
  PStatGraph(monitor, xsize, ysize),
  _view(view),
  _collector_index(collector_index)
{
  _scroll_mode = pstats_scroll_mode;

  _next_frame = 0;
  _first_data = true;
  _cursor_pixel = 0;

  _time_width = 20.0;
  _time_height = 1.0/10.0;
  _start_time = 0.0;

  _level_index = 0;

  set_default_vertical_scale();
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
PStatStripChart::
~PStatStripChart() {
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::new_data
//       Access: Public
//  Description: Indicates that new data has become available.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
new_data(int frame_number) {
  // If the new frame is older than the last one we've drawn, we'll
  // need to back up and redraw it.  This can happen when frames
  // arrive out of order from the client.
  _next_frame = min(frame_number, _next_frame);
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::update
//       Access: Public
//  Description: Updates the chart with the latest data.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
update() {
  const PStatClientData *client_data = get_monitor()->get_client_data();

  // Don't bother to update the thread data until we know at least
  // something about the collectors and threads.
  if (client_data->get_num_collectors() != 0 &&
      client_data->get_num_threads() != 0) {
    const PStatThreadData *thread_data = _view.get_thread_data();
    if (!thread_data->is_empty()) {
      int latest = thread_data->get_latest_frame_number();
      
      if (latest > _next_frame) {
	draw_frames(_next_frame, latest);
      }
      _next_frame = latest;
      
      // Clean out the old data.
      double oldest_time = 
	thread_data->get_frame(latest).get_start() - _time_width;
      
      Data::iterator di;
      di = _data.begin();
      while (di != _data.end() && 
	     thread_data->get_frame((*di).first).get_start() < oldest_time) {
	_data.erase(di);
	di = _data.begin();
      }
    }
  }

  if (_level_index != _view.get_level_index()) {
    update_labels();
  }

  idle();
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::get_collector_under_pixel
//       Access: Public
//  Description: Return the collector index associated with the
//               particular band of color at the indicated pixel
//               location, or -1 if no band of color was at the pixel.
////////////////////////////////////////////////////////////////////
int PStatStripChart::
get_collector_under_pixel(int xpoint, int ypoint) {
  // First, we need to know what frame it was; to know that, we need
  // to determine the time corresponding to the x pixel.
  double time = pixel_to_timestamp(xpoint);
  
  // Now use that time to determine the frame.
  const PStatThreadData *thread_data = _view.get_thread_data();
  int frame_number = thread_data->get_frame_number_at_time(time);

  // And now we can determine which collector within the frame,
  // based on the time height.
  const FrameData &frame = get_frame_data(frame_number);
  double overall_time = 0.0;
  int y = get_ysize();

  FrameData::const_iterator fi;
  for (fi = frame.begin(); fi != frame.end(); ++fi) {
    const ColorData &cd = (*fi);
    overall_time += cd._net_time;
    y = height_to_pixel(overall_time);
    if (y <= ypoint) {
      return cd._collector_index;
    }
  }

  return -1;
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::get_frame_data
//       Access: Protected
//  Description: Returns the cached FrameData associated with the
//               given frame number.  This describes the lengths of
//               the color bands for a single vertical stripe in the
//               chart.
////////////////////////////////////////////////////////////////////
const PStatStripChart::FrameData &PStatStripChart::
get_frame_data(int frame_number) {
  Data::const_iterator di;
  di = _data.find(frame_number);
  if (di != _data.end()) {
    return (*di).second;
  }

  const PStatThreadData *thread_data = _view.get_thread_data();
  _view.set_to_frame(thread_data->get_frame(frame_number));

  FrameData &data = _data[frame_number];

  const PStatViewLevel *level = _view.get_level(_collector_index);
  int num_children = level->get_num_children();
  for (int i = 0; i < num_children; i++) {
    const PStatViewLevel *child = level->get_child(i);
    ColorData cd;
    cd._collector_index = child->get_collector();
    cd._net_time = child->get_net_time();
    if (cd._net_time != 0.0) {
      data.push_back(cd);
    }
  }

  // Also, there might be some time in the overall Collector that
  // wasn't included in all of the children.
  ColorData cd;
  cd._collector_index = level->get_collector();
  cd._net_time = level->get_time_alone();
  if (cd._net_time != 0.0) {
    data.push_back(cd);
  }

  return data;
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::changed_size
//       Access: Protected
//  Description: To be called by the user class when the widget size
//               has changed.  This updates the chart's internal data
//               and causes it to issue redraw commands to reflect the
//               new size.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
changed_size(int xsize, int ysize) {
  if (xsize != _xsize || ysize != _ysize) {
    _cursor_pixel = xsize * _cursor_pixel / _xsize;
    _xsize = xsize;
    _ysize = ysize;
    
    if (!_first_data) {
      if (_scroll_mode) {
	draw_pixels(0, _xsize);
	
      } else {
	// Redraw the stats that were there before.
	double old_start_time = _start_time;
	
	// Back up a bit to draw the stuff to the right of the cursor.
	_start_time -= _time_width;
	draw_pixels(_cursor_pixel, _xsize);
	
	// Now draw the stuff to the left of the cursor.
	_start_time = old_start_time;
	draw_pixels(0, _cursor_pixel);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::force_redraw
//       Access: Protected
//  Description: To be called by the user class when the whole thing
//               needs to be redrawn for some reason.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
force_redraw() {
  if (!_first_data) {
    draw_pixels(0, _xsize);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::force_reset
//       Access: Protected
//  Description: To be called by the user class to cause the chart to
//               reset to empty and start filling again.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
force_reset() {
  clear_region();
  _first_data = true;
}


////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::clear_region
//       Access: Protected, Virtual
//  Description: Should be overridden by the user class to wipe out
//               the entire strip chart region.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
clear_region() {
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::copy_region
//       Access: Protected, Virtual
//  Description: Should be overridden by the user class to copy a
//               region of the chart from one part of the chart to
//               another.  This is used to implement scrolling.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
copy_region(int, int, int) {
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::begin_draw
//       Access: Protected, Virtual
//  Description: Should be overridden by the user class.  This hook
//               will be called before drawing any color bars in the
//               strip chart; it gives the pixel range that's about to
//               be redrawn.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
begin_draw(int, int) {
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::draw_slice
//       Access: Protected, Virtual
//  Description: Should be overridden by the user class to draw a
//               single vertical slice in the strip chart at the
//               indicated pixel, with the data for the indicated
//               frame.  Call get_frame_data() to get the actual color
//               data for the given frame_number.  This call will only
//               be made between a corresponding call to begin_draw()
//               and end_draw().
////////////////////////////////////////////////////////////////////
void PStatStripChart::
draw_slice(int, int) {
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::draw_empty
//       Access: Protected, Virtual
//  Description: This is similar to draw_slice(), except it should
//               draw a vertical line of the background color to
//               represent a portion of the chart that has no data.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
draw_empty(int) {
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::draw_cursor
//       Access: Protected, Virtual
//  Description: This is similar to draw_slice(), except that it
//               should draw the black vertical stripe that represents
//               the current position when not in scrolling mode.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
draw_cursor(int) {
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::end_draw
//       Access: Protected, Virtual
//  Description: Should be overridden by the user class.  This hook
//               will be called after drawing a series of color bars
//               in the strip chart; it gives the pixel range that
//               was just redrawn.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
end_draw(int, int) {
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::idle
//       Access: Protected, Virtual
//  Description: Should be overridden by the user class to perform any
//               other updates might be necessary after the color bars
//               have been redrawn.  For instance, it could check the
//               state of _labels_changed, and redraw the labels if it
//               is true.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
idle() {
}


// STL function object for sorting labels in order by the collector's
// sort index, used in update_labels(), below.
class SortCollectorLabels {
public:
  SortCollectorLabels(const PStatClientData *client_data) :
    _client_data(client_data) {
  }
  bool operator () (int a, int b) const {
    // By casting the sort numbers to unsigned ints, we cheat and make
    // -1 appear to be a very large positive integer, thus placing
    // collectors with a -1 sort value at the very end.
    return 
      (unsigned int)_client_data->get_collector_def(a)._sort <
      (unsigned int)_client_data->get_collector_def(b)._sort;
  }
  const PStatClientData *_client_data;
};

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::update_labels
//       Access: Protected
//  Description: Resets the list of labels.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
update_labels() {
  const PStatViewLevel *level = _view.get_level(_collector_index);
  _labels.clear();
  
  int num_children = level->get_num_children();
  for (int i = 0; i < num_children; i++) {
    const PStatViewLevel *child = level->get_child(i);
    int collector = child->get_collector();
    _labels.push_back(collector);
  }

  SortCollectorLabels sort_labels(get_monitor()->get_client_data());
  sort(_labels.begin(), _labels.end(), sort_labels);

  int collector = level->get_collector();
  _labels.push_back(collector);

  _labels_changed = true;
  _level_index = _view.get_level_index();
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::normal_guide_bars
//       Access: Protected, Virtual
//  Description: Calls update_guide_bars with parameters suitable to
//               this kind of graph.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
normal_guide_bars() {
  update_guide_bars(4, _time_height);
}


////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::draw_frames
//       Access: Private
//  Description: Draws the levels for the indicated frame range.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
draw_frames(int first_frame, int last_frame) {
  const PStatThreadData *thread_data = _view.get_thread_data();

  last_frame = min(last_frame, thread_data->get_latest_frame_number());

  if (_first_data) {
    if (_scroll_mode) {
      _start_time = 
	thread_data->get_frame(last_frame).get_start() - _time_width;
    } else {
      _start_time = thread_data->get_frame(first_frame).get_start();
      _cursor_pixel = 0;
    }
  }

  int first_pixel;
  if (thread_data->has_frame(first_frame)) {
    first_pixel =
      timestamp_to_pixel(thread_data->get_frame(first_frame).get_start());
  } else {
    first_pixel = 0;
  }

  int last_pixel = 
    timestamp_to_pixel(thread_data->get_frame(last_frame).get_start());

  if (_first_data && !_scroll_mode) {
    first_pixel = min(_cursor_pixel, first_pixel);
  }
  _first_data = false;

  if (last_pixel - first_pixel >= _xsize) {
    // If we're drawing the whole thing all in this one swoop, just
    // start over.
    _start_time = thread_data->get_frame(last_frame).get_start() - _time_width;
    first_pixel = 0;
    last_pixel = _xsize;
  }
    
  if (last_pixel <= _xsize) {
    // It all fits in one block.
    _cursor_pixel = last_pixel;
    draw_pixels(first_pixel, last_pixel);

  } else {
    if (_scroll_mode) {
      // In scrolling mode, slide the world back.
      int slide_pixels = last_pixel - _xsize;
      copy_region(slide_pixels, first_pixel, 0);
      first_pixel -= slide_pixels;
      last_pixel -= slide_pixels;
      _start_time += (double)slide_pixels / (double)_xsize * _time_width;
      draw_pixels(first_pixel, last_pixel);

    } else {
      // In wrapping mode, do it in two blocks.
      _cursor_pixel = -1;
      draw_pixels(first_pixel, _xsize);
      _start_time = pixel_to_timestamp(_xsize);
      last_pixel -= _xsize;
      _cursor_pixel = last_pixel;
      draw_pixels(0, last_pixel);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PStatStripChart::draw_pixels
//       Access: Private
//  Description: Draws the levels for the indicated pixel range.
////////////////////////////////////////////////////////////////////
void PStatStripChart::
draw_pixels(int first_pixel, int last_pixel) {
  begin_draw(first_pixel, last_pixel);
  const PStatThreadData *thread_data = _view.get_thread_data();

  int frame_number = -1;
  for (int x = first_pixel; x <= last_pixel; x++) {
    if (x == _cursor_pixel && !_scroll_mode) {
      draw_cursor(x);

    } else {
      double time = pixel_to_timestamp(x);
      frame_number = thread_data->get_frame_number_at_time(time, frame_number);
      
      if (thread_data->has_frame(frame_number)) {
	draw_slice(x, frame_number);
      } else {
	draw_empty(x);
      }
    }
  }
  end_draw(first_pixel, last_pixel);
}
