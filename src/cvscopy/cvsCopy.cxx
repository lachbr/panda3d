// Filename: cvsCopy.cxx
// Created by:  drose (31Oct00)
// 
////////////////////////////////////////////////////////////////////

#include "cvsCopy.h"
#include "cvsSourceDirectory.h"

#include <notify.h>

////////////////////////////////////////////////////////////////////
//     Function: CVSCopy::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
CVSCopy::
CVSCopy() {
  _model_dirname = ".";
  _key_filename = "Sources.pp";
  _cvs_binary = "cvs";
  _model_dir = (CVSSourceDirectory *)NULL;
  _map_dir = (CVSSourceDirectory *)NULL;

  clear_runlines();
  add_runline("[opts] file [file ... ]");

  add_option
    ("f", "", 80,
     "Force copy to happen without any input from the user.  If a file "
     "with the same name exists anywhere in the source hierarchy, it will "
     "be overwritten without prompting; if a file does not yet exist, it "
     "will be created in the directory named by -d or by -m, as appropriate.",
     &CVSCopy::dispatch_none, &_force);

  add_option
    ("i", "", 80,
     "The opposite of -f, this will prompt the user before each action.  "
     "The default is only to prompt the user when an action is ambiguous.",
     &CVSCopy::dispatch_none, &_interactive);

  add_option
    ("d", "dirname", 80, 
     "Copy model files that are not already present somewhere in the tree "
     "to the indicated directory.  The default is the current directory.",
     &CVSCopy::dispatch_filename, &_got_model_dirname, &_model_dirname);

  add_option
    ("m", "dirname", 80, 
     "Copy texture map files to the indicated directory.  The default "
     "is src/maps from the root directory.",
     &CVSCopy::dispatch_filename, &_got_map_dirname, &_map_dirname);

  add_option
    ("root", "dirname", 80, 
     "Specify the root of the CVS source hierarchy.  The default is to "
     "use the ppremake convention of locating the directory containing "
     "Package.pp.",
     &CVSCopy::dispatch_filename, &_got_root_dirname, &_root_dirname);

  add_option
    ("key", "filename", 80, 
     "Specify the name of the file that must exist in each directory for "
     "it to be considered part of the CVS source hierarchy.  The default "
     "is the ppremake convention, \"Sources.pp\".  Other likely candidates "
     "are \"CVS\" to search the entire CVS hierarchy, or \".\" to include "
     "all subdirectories.",
     &CVSCopy::dispatch_filename, NULL, &_key_filename);

  add_option
    ("nc", "", 80, 
     "Do not attempt to add newly-created files to CVS.  The default "
     "is to add them.",
     &CVSCopy::dispatch_none, &_no_cvs);

  add_option
    ("cvs", "cvs_binary", 80, 
     "Specify how to run the cvs program for adding newly-created files.  "
     "The default is simply \"cvs\".",
     &CVSCopy::dispatch_string, NULL, &_cvs_binary);
}

////////////////////////////////////////////////////////////////////
//     Function: CVSCopy::import
//       Access: Public
//  Description: Checks for the given filename somewhere in the
//               directory hierarchy, and chooses a place to import
//               it.  Copies the file by calling copy_file().
//
//               Type is an integer number that is defined by the
//               derivated class; CVSCopy simply passes it unchanged
//               to copy_file().  It presumably gives the class a hint
//               as to how the file should be copied.  Suggested_dir
//               is the suggested directory in which to copy the file,
//               if it does not already exist elsewhere.
//
//               On success, returns the CVSSourceDirectory it was
//               actually copied to.  On failure, returns NULL.
////////////////////////////////////////////////////////////////////
CVSSourceDirectory *CVSCopy::
import(const Filename &source, int type, CVSSourceDirectory *suggested_dir) {
  CopiedFiles::const_iterator ci;
  ci = _copied_files.find(source);
  if (ci != _copied_files.end()) {
    // We have already copied this file.
    return (*ci).second;
  }

  if (!source.exists()) {
    cerr << "Source filename " << source << " does not exist!\n";
    return (CVSSourceDirectory *)NULL;
  }

  string basename = source.get_basename();

  CVSSourceDirectory *dir = 
    _tree.choose_directory(basename, suggested_dir, _force, _interactive);
  nassertr(dir != (CVSSourceDirectory *)NULL, dir);

  Filename dest = dir->get_fullpath() + "/" + basename;

  _copied_files[source] = dir;

  bool new_file = !dest.exists();
  if (!copy_file(source, dest, dir, type, new_file)) {
    return (CVSSourceDirectory *)NULL;
  }
  if (new_file) {
    create_file(dest);
  }

  return dir;
}


////////////////////////////////////////////////////////////////////
//     Function: CVSCopy::handle_args
//       Access: Protected, Virtual
//  Description: Does something with the additional arguments on the
//               command line (after all the -options have been
//               parsed).  Returns true if the arguments are good,
//               false otherwise.
////////////////////////////////////////////////////////////////////
bool CVSCopy::
handle_args(Args &args) {
  if (args.empty()) {
    nout << "You must specify the file(s) to copy from on the command line.\n";
    return false;
  }

  _source_files = args;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: CVSCopy::post_command_line
//       Access: Protected, Virtual
//  Description: This is called after the command line has been
//               completely processed, and it gives the program a
//               chance to do some last-minute processing and
//               validation of the options and arguments.  It should
//               return true if everything is fine, false if there is
//               an error.
////////////////////////////////////////////////////////////////////
bool CVSCopy::
post_command_line() {
  if (!scan_hierarchy()) {
    return false;
  }

  _model_dir = _tree.find_directory(_model_dirname);
  if (_model_dir == (CVSSourceDirectory *)NULL) {
    if (_got_model_dirname) {
      nout << "Warning: model directory " << _model_dirname
	   << " is not within the source hierarchy.\n";
    }
  }

  if (_got_map_dirname) {
    _map_dir = _tree.find_directory(_map_dirname);

    if (_map_dir == (CVSSourceDirectory *)NULL) {
      nout << "Warning: map directory " << _map_dirname
	   << " is not within the source hierarchy.\n";
    }

  } else {
    _map_dir = _tree.find_relpath("src/maps");

    if (_map_dir == (CVSSourceDirectory *)NULL) {
      nout << "Warning: no directory " << _tree.get_root_dirname()
	   << "/src/maps.\n";
      _map_dir = _model_dir;
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: CVSCopy::create_file
//       Access: Protected
//  Description: Invokes CVS to add the indicated filename to the
//               repository, if the user so requested.  Returns true
//               if successful, false if there is an error.
////////////////////////////////////////////////////////////////////
bool CVSCopy::
create_file(const Filename &filename) {
  if (_no_cvs) {
    return true;
  }

  if (!CVSSourceTree::temp_chdir(filename.get_dirname())) {
    nout << "Invalid directory: " << filename.get_dirname() << "\n";
    return false;
  }

  string command = _cvs_binary + " add " + filename.get_basename();
  nout << command << "\n";
  int result = system(command.c_str());

  CVSSourceTree::restore_cwd();

  if (result != 0) {
    nout << "Failure invoking cvs.\n";
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: CVSCopy::scan_hierarchy
//       Access: Private
//  Description: Starts the scan of the source hierarchy.  This
//               identifies all of the files in the source hierarchy
//               we're to copy these into, so we can guess where
//               referenced files should be placed.  Returns true if
//               everything is ok, false if there is an error.
////////////////////////////////////////////////////////////////////
bool CVSCopy::
scan_hierarchy() {
  if (!_got_root_dirname) {
    // If we didn't get a root directory name, find the directory
    // above this one that contains the file "Package.pp".
    if (!scan_for_root(_model_dirname)) {
      return false;
    }
  }

  _tree.set_root(_root_dirname);
  nout << "Root is " << _tree.get_root_fullpath() << "\n";

  return _tree.scan(_key_filename);
}

////////////////////////////////////////////////////////////////////
//     Function: CVSCopy::scan_for_root
//       Access: Private
//  Description: Searches for the root of the source directory by
//               looking for the parent directory that contains
//               "Package.pp".  Returns true on success, false on
//               failure.
////////////////////////////////////////////////////////////////////
bool CVSCopy::
scan_for_root(const string &dirname) {
  Filename sources = dirname + "/Sources.pp";
  if (!sources.exists()) {
    nout << "Couldn't find " << sources << " in source directory.\n";
    return false;
  }
  Filename package = dirname + "/Package.pp";
  if (package.exists()) {
    // Here's the root!
    _root_dirname = dirname;
    return true;
  }

  return scan_for_root(dirname + "/..");
}
