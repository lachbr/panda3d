// Filename: gtkStatsStripChart.h
// Created by:  drose (14Jul00)
// 
////////////////////////////////////////////////////////////////////

#ifndef GTKSTATSSTRIPCHART_H
#define GTKSTATSSTRIPCHART_H

#include <pandatoolbase.h>

#include "gtkStatsMonitor.h"

#include <pStatStripChart.h>
#include <pointerTo.h>

#include <gtk--.h>
#include <map>

class PStatView;
class GtkStatsGuide;

////////////////////////////////////////////////////////////////////
// 	 Class : GtkStatsStripChart
// Description : A special widget that draws a strip chart, given a
//               view.
////////////////////////////////////////////////////////////////////
class GtkStatsStripChart : public Gtk::DrawingArea, public PStatStripChart {
public:
  GtkStatsStripChart(GtkStatsMonitor *monitor,
		     PStatView &view, int collector_index,
		     int xsize, int ysize);

  void mark_dead();

  Gtk::Alignment *get_labels();
  GtkStatsGuide *get_guide();

  Gdk_GC get_collector_gc(int collector_index);

  // This signal is thrown when the user float-clicks on a label or
  // on a band of color.
  SigC::Signal1<void, int> collector_picked;

private:
  virtual void clear_region();
  virtual void copy_region(int start_x, int end_x, int dest_x);
  virtual void draw_slice(int x, int frame_number);
  virtual void draw_empty(int x);
  virtual void draw_cursor(int x);
  virtual void end_draw(int from_x, int to_x);
  virtual void idle();

  virtual gint configure_event_impl(GdkEventConfigure *event);
  virtual gint expose_event_impl(GdkEventExpose *event);
  virtual gint button_press_event_impl(GdkEventButton *button);

  void pack_labels();
  void setup_white_gc();

private:
  // Backing pixmap for drawing area.
  Gdk_Pixmap _pixmap;

  // Graphics contexts for fg/bg.  We don't use the contexts defined
  // in the style, because that would probably interfere with the
  // visibility of the strip chart.
  Gdk_GC _white_gc;
  Gdk_GC _black_gc;
  Gdk_GC _dark_gc;
  Gdk_GC _light_gc;

  // Table of graphics contexts for our various collectors.
  typedef map<int, Gdk_GC> GCs;
  GCs _gcs;

  Gtk::Alignment *_label_align;
  Gtk::VBox *_label_box;
  GtkStatsGuide *_guide;
  bool _is_dead;
};

#include "gtkStatsStripChart.I"

#endif

