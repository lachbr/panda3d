// Filename: gtkStatsChartMenu.cxx
// Created by:  drose (16Jan06)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#include "gtkStatsChartMenu.h"
#include "gtkStatsMonitor.h"

////////////////////////////////////////////////////////////////////
//     Function: GtkStatsChartMenu::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
GtkStatsChartMenu::
GtkStatsChartMenu(GtkStatsMonitor *monitor, int thread_index) :
  _monitor(monitor),
  _thread_index(thread_index)
{
  _menu = CreatePopupMenu();
  do_update();
}

////////////////////////////////////////////////////////////////////
//     Function: GtkStatsChartMenu::Destructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
GtkStatsChartMenu::
~GtkStatsChartMenu() {
}

////////////////////////////////////////////////////////////////////
//     Function: GtkStatsChartMenu::get_menu_handle
//       Access: Public
//  Description: Returns the Windows menu handle for this particular
//               menu.
////////////////////////////////////////////////////////////////////
HMENU GtkStatsChartMenu::
get_menu_handle() {
  return _menu;
}

////////////////////////////////////////////////////////////////////
//     Function: GtkStatsChartMenu::add_to_menu_bar
//       Access: Public
//  Description: Adds the menu to the end of the indicated menu bar.
////////////////////////////////////////////////////////////////////
void GtkStatsChartMenu::
add_to_menu_bar(HMENU menu_bar, int before_menu_id) {
  const PStatClientData *client_data = _monitor->get_client_data();
  string thread_name;
  if (_thread_index == 0) {
    // A special case for the main thread.
    thread_name = "Graphs";
  } else {
    thread_name = client_data->get_thread_name(_thread_index);
  }

  MENUITEMINFO mii;
  memset(&mii, 0, sizeof(mii));
  mii.cbSize = sizeof(mii);

  mii.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_SUBMENU; 
  mii.fType = MFT_STRING; 
  mii.hSubMenu = _menu; 
  mii.dwTypeData = (char *)thread_name.c_str(); 
  InsertMenuItem(menu_bar, before_menu_id, FALSE, &mii);
}

////////////////////////////////////////////////////////////////////
//     Function: GtkStatsChartMenu::check_update
//       Access: Public
//  Description: Checks to see if the menu needs to be updated
//               (e.g. because of new data from the client), and
//               updates it if necessary.
////////////////////////////////////////////////////////////////////
void GtkStatsChartMenu::
check_update() {
  PStatView &view = _monitor->get_view(_thread_index);
  if (view.get_level_index() != _last_level_index) {
    do_update();
  }
}
 
////////////////////////////////////////////////////////////////////
//     Function: GtkStatsChartMenu::do_update
//       Access: Public
//  Description: Unconditionally updates the menu with the latest data
//               from the client.
////////////////////////////////////////////////////////////////////
void GtkStatsChartMenu::
do_update() {
  PStatView &view = _monitor->get_view(_thread_index);
  _last_level_index = view.get_level_index();

  // First, remove all of the old entries from the menu.
  int num_items = GetMenuItemCount(_menu);
  for (int i = num_items - 1; i >= 0; i--) {
    DeleteMenu(_menu, i, MF_BYPOSITION);
  }

  // Now rebuild the menu with the new set of entries.

  // The menu item(s) for the thread's frame time goes first.
  add_view(_menu, view.get_top_level(), false);

  bool needs_separator = true;
  MENUITEMINFO mii;
  memset(&mii, 0, sizeof(mii));
  mii.cbSize = sizeof(mii);

  // And then the menu item(s) for each of the level values.
  const PStatClientData *client_data = _monitor->get_client_data();
  int num_toplevel_collectors = client_data->get_num_toplevel_collectors();
  for (int tc = 0; tc < num_toplevel_collectors; tc++) {
    int collector = client_data->get_toplevel_collector(tc);
    if (client_data->has_collector(collector) && 
        client_data->get_collector_has_level(collector)) {

      // We put a separator between the above frame collector and the
      // first level collector.
      if (needs_separator) {
        mii.fMask = MIIM_FTYPE; 
        mii.fType = MFT_SEPARATOR; 
        InsertMenuItem(_menu, GetMenuItemCount(_menu), TRUE, &mii);

        needs_separator = false;
      }

      PStatView &level_view = _monitor->get_level_view(collector, _thread_index);
      add_view(_menu, level_view.get_top_level(), true);
    }
  }

  // Also a menu item for a piano roll (following a separator).
  mii.fMask = MIIM_FTYPE; 
  mii.fType = MFT_SEPARATOR; 
  InsertMenuItem(_menu, GetMenuItemCount(_menu), TRUE, &mii);

  GtkStatsMonitor::MenuDef menu_def(_thread_index, -1, false);
  int menu_id = _monitor->get_menu_id(menu_def);

  mii.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID; 
  mii.fType = MFT_STRING; 
  mii.wID = menu_id;
  mii.dwTypeData = "Piano Roll";
  InsertMenuItem(_menu, GetMenuItemCount(_menu), TRUE, &mii);
}

////////////////////////////////////////////////////////////////////
//     Function: GtkStatsChartMenu::add_view
//       Access: Private
//  Description: Adds a new entry or entries to the menu for the
//               indicated view and its children.
////////////////////////////////////////////////////////////////////
void GtkStatsChartMenu::
add_view(HMENU parent_menu, const PStatViewLevel *view_level, bool show_level) {
  int collector = view_level->get_collector();

  const PStatClientData *client_data = _monitor->get_client_data();
  string collector_name = client_data->get_collector_name(collector);

  GtkStatsMonitor::MenuDef menu_def(_thread_index, collector, show_level);
  int menu_id = _monitor->get_menu_id(menu_def);

  MENUITEMINFO mii;
  memset(&mii, 0, sizeof(mii));
  mii.cbSize = sizeof(mii);

  mii.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID; 
  mii.fType = MFT_STRING; 
  mii.wID = menu_id;
  mii.dwTypeData = (char *)collector_name.c_str(); 
  InsertMenuItem(parent_menu, GetMenuItemCount(parent_menu), TRUE, &mii);

  int num_children = view_level->get_num_children();
  if (num_children > 1) {
    // If the collector has more than one child, add a menu entry to go
    // directly to each of its children.
    HMENU submenu = CreatePopupMenu();
    string submenu_name = collector_name + " components";

    mii.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_SUBMENU;
    mii.fType = MFT_STRING; 
    mii.hSubMenu = submenu;
    mii.dwTypeData = (char *)submenu_name.c_str(); 
    InsertMenuItem(parent_menu, GetMenuItemCount(parent_menu), TRUE, &mii);

    // Reverse the order since the menus are listed from the top down;
    // we want to be visually consistent with the graphs, which list
    // these labels from the bottom up.
    for (int c = num_children - 1; c >= 0; c--) {
      add_view(submenu, view_level->get_child(c), show_level);
    }
  }
}
