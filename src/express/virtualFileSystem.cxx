// Filename: virtualFileSystem.cxx
// Created by:  drose (03Aug02)
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

#include "virtualFileSystem.h"
#include "virtualFileMount.h"
#include "virtualFileMountMultifile.h"
#include "virtualFileMountSystem.h"
#include "dSearchPath.h"
#include "dcast.h"
#include "config_express.h"
#include "executionEnvironment.h"
#include "pset.h"

VirtualFileSystem *VirtualFileSystem::_global_ptr = NULL;


////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::Constructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
VirtualFileSystem::
VirtualFileSystem() {
  _cwd = "/";
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::Destructor
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
VirtualFileSystem::
~VirtualFileSystem() {
  unmount_all();
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::mount
//       Access: Published
//  Description: Mounts the indicated Multifile at the given mount
//               point.  If flags contains MF_owns_pointer, the
//               Multifile will be deleted when it is eventually
//               unmounted.
////////////////////////////////////////////////////////////////////
bool VirtualFileSystem::
mount(Multifile *multifile, const string &mount_point, int flags) {
  VirtualFileMountMultifile *mount = 
    new VirtualFileMountMultifile(this, multifile, 
                                  normalize_mount_point(mount_point),
                                  flags);
  _mounts.push_back(mount);
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::mount
//       Access: Published
//  Description: Mounts the indicated system file or directory at the
//               given mount point.  If the named file is a directory,
//               mounts the directory.  If the named file is a
//               Multifile, mounts it as a Multifile.  Returns true on
//               success, false on failure.
//
//               A given system directory may be mounted to multiple
//               different mount point, and the same mount point may
//               share multiple system directories.  In the case of
//               ambiguities, the most-recently mounted system wins.
////////////////////////////////////////////////////////////////////
bool VirtualFileSystem::
mount(const Filename &physical_filename, const string &mount_point, 
      int flags) {
  if (!physical_filename.exists()) {
    express_cat.warning()
      << "Attempt to mount " << physical_filename << ", not found.\n";
    return false;
  }

  if (physical_filename.is_directory()) {
    flags &= ~MF_owns_pointer;
    VirtualFileMountSystem *mount =
      new VirtualFileMountSystem(this, physical_filename, 
                                 normalize_mount_point(mount_point),
                                 flags);
    _mounts.push_back(mount);
    return true;

  } else {
    // It's not a directory; it must be a Multifile.
    Multifile *multifile = new Multifile;

    // For now these are always opened read only.  Maybe later we'll
    // support read-write on Multifiles.
    flags |= MF_read_only;
    if (!multifile->open_read(physical_filename)) {
      delete multifile;
      return false;
    }

    // We want to delete this pointer when we're done.
    flags |= MF_owns_pointer;
    return mount(multifile, mount_point, flags);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::unmount
//       Access: Published
//  Description: Unmounts all appearances of the indicated Multifile
//               from the file system.  Returns the number of
//               appearances unmounted.
////////////////////////////////////////////////////////////////////
int VirtualFileSystem::
unmount(Multifile *multifile) {
  Mounts::iterator ri, wi;
  wi = ri = _mounts.begin();
  while (ri != _mounts.end()) {
    VirtualFileMount *mount = (*ri);
    (*wi) = mount;

    if (mount->is_exact_type(VirtualFileMountMultifile::get_class_type())) {
      VirtualFileMountMultifile *mmount = 
        DCAST(VirtualFileMountMultifile, mount);
      if (mmount->get_multifile() == multifile) {
        // Remove this one.  Don't increment wi.
        delete mount;
      } else {
        // Don't remove this one.
        ++wi;
      }
    } else {
      // Don't remove this one.
      ++wi;
    }
    ++ri;
  }

  int num_removed = _mounts.end() - wi;
  _mounts.erase(wi, _mounts.end());
  return num_removed;
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::unmount
//       Access: Published
//  Description: Unmounts all appearances of the indicated physical
//               filename (either a directory name or a Multifile
//               name) from the file system.  Returns the number of
//               appearances unmounted.
////////////////////////////////////////////////////////////////////
int VirtualFileSystem::
unmount(const Filename &physical_filename) {
  Mounts::iterator ri, wi;
  wi = ri = _mounts.begin();
  while (ri != _mounts.end()) {
    VirtualFileMount *mount = (*ri);
    (*wi) = mount;

    if (mount->get_physical_filename() == physical_filename) {
      // Remove this one.  Don't increment wi.
      delete mount;
    } else {
      // Don't remove this one.
      ++wi;
    }
    ++ri;
  }

  int num_removed = _mounts.end() - wi;
  _mounts.erase(wi, _mounts.end());
  return num_removed;
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::unmount_point
//       Access: Published
//  Description: Unmounts all systems attached to the given mount
//               point from the file system.  Returns the number of
//               appearances unmounted.
////////////////////////////////////////////////////////////////////
int VirtualFileSystem::
unmount_point(const string &mount_point) {
  Filename nmp = normalize_mount_point(mount_point);
  Mounts::iterator ri, wi;
  wi = ri = _mounts.begin();
  while (ri != _mounts.end()) {
    VirtualFileMount *mount = (*ri);
    (*wi) = mount;

    if (mount->get_mount_point() == nmp) {
      // Remove this one.  Don't increment wi.
      delete mount;
    } else {
      // Don't remove this one.
      ++wi;
    }
    ++ri;
  }

  int num_removed = _mounts.end() - wi;
  _mounts.erase(wi, _mounts.end());
  return num_removed;
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::unmount_all
//       Access: Published
//  Description: Unmounts all files from the file system.  Returns the
//               number of systems unmounted.
////////////////////////////////////////////////////////////////////
int VirtualFileSystem::
unmount_all() {
  Mounts::iterator mi;
  for (mi = _mounts.begin(); mi != _mounts.end(); ++mi) {
    VirtualFileMount *mount = (*mi);
    delete mount;
  }

  int num_removed = _mounts.size();
  _mounts.clear();
  return num_removed;
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::chdir
//       Access: Published
//  Description: Changes the current directory.  This is used to
//               resolve relative pathnames in get_file() and/or
//               find_file().  Returns true if successful, false
//               otherwise.
//
//               This accepts a string rather than a Filename simply
//               for programmer convenience from the Python prompt.
////////////////////////////////////////////////////////////////////
bool VirtualFileSystem::
chdir(const string &new_directory) {
  if (new_directory == "/") {
    // We can always return to the root.
    _cwd = new_directory;
    return true;
  }

  PT(VirtualFile) file = get_file(new_directory);
  if (file != (VirtualFile *)NULL && file->is_directory()) {
    _cwd = file->get_filename();
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::get_cwd
//       Access: Published
//  Description: Returns the current directory name.  See chdir().
////////////////////////////////////////////////////////////////////
const Filename &VirtualFileSystem::
get_cwd() const {
  return _cwd;
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::get_file
//       Access: Published
//  Description: Looks up the file by the indicated name in the file
//               system.  Returns a VirtualFile pointer representing
//               the file if it is found, or NULL if it is not.
////////////////////////////////////////////////////////////////////
PT(VirtualFile) VirtualFileSystem::
get_file(const Filename &filename) const {
  nassertr(!filename.empty(), NULL);
  Filename pathname(filename);
  if (pathname.is_local()) {
    pathname = Filename(_cwd, filename);
  }
  pathname.standardize();
  string strpath = pathname.get_fullpath().substr(1);

  // Now scan all the mount points, from the back (since later mounts
  // override more recent ones), until a match is found.
  PT(VirtualFile) found_file = NULL;
  VirtualFileComposite *composite_file = NULL;

  Mounts::const_reverse_iterator rmi;
  for (rmi = _mounts.rbegin(); rmi != _mounts.rend(); ++rmi) {
    VirtualFileMount *mount = (*rmi);
    string mount_point = mount->get_mount_point();
    if (strpath == mount_point) {
      // Here's an exact match on the mount point.  This filename is
      // the root directory of this mount object.
      if (found_match(found_file, composite_file, mount, "")) {
        return found_file;
      }

    } else if (mount_point.empty()) {
      // This is the root mount point; all files are in here.
      if (mount->has_file(strpath)) {
        // Bingo!
        if (found_match(found_file, composite_file, mount, strpath)) {
          return found_file;
        }
      }            

    } else if (strpath.length() > mount_point.length() &&
               strpath.substr(0, mount_point.length()) == mount_point &&
               strpath[mount_point.length()] == '/') {
      // This pathname falls within this mount system.
      Filename local_filename = strpath.substr(mount_point.length() + 1);
      if (mount->has_file(local_filename)) {
        // Bingo!
        if (found_match(found_file, composite_file, mount, local_filename)) {
          return found_file;
        }
      }            
    }
  }
  return found_file;
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::find_file
//       Access: Published
//  Description: Uses the indicated search path to find the file
//               within the file system.  Returns the first occurrence
//               of the file found, or NULL if the file cannot be
//               found.
////////////////////////////////////////////////////////////////////
PT(VirtualFile) VirtualFileSystem::
find_file(const Filename &filename, const DSearchPath &searchpath) const {
  if (!filename.is_local()) {
    return get_file(filename);
  }

  int num_directories = searchpath.get_num_directories();
  for (int i = 0; i < num_directories; i++) {
    Filename match(searchpath.get_directory(i), filename);
    PT(VirtualFile) found_file = get_file(match);
    if (found_file != (VirtualFile *)NULL) {
      return found_file;
    }
  }

  return NULL;
}


////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::resolve_filename
//       Access: Public
//  Description: Searches the given search path for the filename.  If
//               it is found, updates the filename to the full
//               pathname found and returns true; otherwise, returns
//               false.
////////////////////////////////////////////////////////////////////
bool VirtualFileSystem::
resolve_filename(Filename &filename,
                 const DSearchPath &searchpath,
                 const string &default_extension) const {
  PT(VirtualFile) found;

  if (filename.is_local()) {
    found = find_file(filename.get_fullpath(), searchpath);

    if (found.is_null()) {
      // We didn't find it with the given extension; can we try the
      // default extension?
      if (filename.get_extension().empty() && !default_extension.empty()) {
        Filename try_ext = filename;
        try_ext.set_extension(default_extension);
        found = find_file(try_ext.get_fullpath(), searchpath);
      }
    }

  } else {
    if (exists(filename)) {
      // The full pathname exists.  Return true.
      return true;

    } else {
      // The full pathname doesn't exist with the given extension;
      // does it exist with the default extension?
      if (filename.get_extension().empty() && !default_extension.empty()) {
        Filename try_ext = filename;
        try_ext.set_extension(default_extension);
        found = get_file(try_ext);
      }
    }
  }

  if (!found.is_null()) {
    filename = found->get_filename();
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::write
//       Access: Published
//  Description: 
////////////////////////////////////////////////////////////////////
void VirtualFileSystem::
write(ostream &out) const {
  Mounts::const_iterator mi;
  for (mi = _mounts.begin(); mi != _mounts.end(); ++mi) {
    VirtualFileMount *mount = (*mi);
    mount->write(out);
  }
}


////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::get_global_ptr
//       Access: Published, Static
//  Description: Returns the default global VirtualFileSystem.  You
//               may create your own personal VirtualFileSystem
//               objects and use them for whatever you like, but Panda
//               will attempt to load models and stuff from this
//               default object.
//
//               Initially, the global VirtualFileSystem is set up to
//               mount the OS filesystem to root; i.e. it is
//               equivalent to the OS filesystem.  This may be
//               subsequently adjusted by the user.
////////////////////////////////////////////////////////////////////
VirtualFileSystem *VirtualFileSystem::
get_global_ptr() {
  if (_global_ptr == (VirtualFileSystem *)NULL) {
    _global_ptr = new VirtualFileSystem;
    
    // Set up the default mounts.  First, there is always the root
    // mount.
    _global_ptr->mount("/", "/", 0);

    // And our initial cwd comes from the environment.
    _global_ptr->chdir(ExecutionEnvironment::get_cwd());

    // Then, we add whatever mounts are listed in the Configrc file.
    Config::ConfigTable::Symbol mounts;
    config_express.GetAll("vfs-mount", mounts);

    // When we use GetAll(), we might inadvertently read duplicate
    // lines.  Filter them out with a set.
    pset<string> already_read;

    Config::ConfigTable::Symbol::iterator si;
    for (si = mounts.begin(); si != mounts.end(); ++si) {
      string mount_desc = (*si).Val();
      if (already_read.insert(mount_desc).second) {

        // The vfs-mount syntax is:

        // vfs-mount system-filename mount-point [options]

        // The last two spaces mark the beginning of the mount point,
        // and of the options, respectively.  There might be multiple
        // spaces in the system filename, which are part of the
        // filename.

        // The last space marks the beginning of the mount point.
        // Spaces before that are part of the system filename.
        size_t space = mount_desc.rfind(' ');
        if (space == string::npos) {
          express_cat.warning()
            << "No space in vfs-mount descriptor: " << mount_desc << "\n";
          
        } else {
          string mount_point = mount_desc.substr(space + 1);
          while (space > 0 && isspace(mount_desc[space - 1])) {
            space--;
          }
          mount_desc = mount_desc.substr(0, space);
          string options;

          space = mount_desc.rfind(' ');
          if (space != string::npos) {
            // If there's another space, we have the optional options field.
            options = mount_point;
            mount_point = mount_desc.substr(space + 1);
            while (space > 0 && isspace(mount_desc[space - 1])) {
              space--;
            }
            mount_desc = mount_desc.substr(0, space);
          }

          mount_desc = ExecutionEnvironment::expand_string(mount_desc);
          Filename physical_filename = Filename::from_os_specific(mount_desc);

          _global_ptr->mount(physical_filename, mount_point, 0);
        }
      }
    }
  }

  return _global_ptr;
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::scan_mount_points
//       Access: Public
//  Description: Adds to names a list of all the mount points in use
//               that are one directory below path, if any.  That is,
//               these are the external files or directories mounted
//               directly to the indicated path.
//
//               The names vector is filled with a set of basenames,
//               the basename part of the mount point.
////////////////////////////////////////////////////////////////////
void VirtualFileSystem::
scan_mount_points(vector_string &names, const Filename &path) const {
  nassertv(!path.empty() && !path.is_local());
  string prefix = path.get_fullpath().substr(1);
  Mounts::const_iterator mi;
  for (mi = _mounts.begin(); mi != _mounts.end(); ++mi) {
    VirtualFileMount *mount = (*mi);
    
    string mount_point = mount->get_mount_point();
    if (prefix.empty()) {
      // The indicated path is the root.  Is the mount point on the
      // root?
      if (mount_point.find('/') == string::npos) {
        // No embedded slashes, so the mount point is only one
        // directory below the root.
        names.push_back(mount_point);
      }
    } else {
      if (mount_point.substr(0, prefix.length()) == prefix &&
          mount_point.length() > prefix.length() &&
          mount_point[prefix.length()] == '/') {
        // This mount point is below the indicated path.  Is it only one
        // directory below?
        string basename = mount_point.substr(prefix.length());
        if (basename.find('/') == string::npos) {
          // No embedded slashes, so it's only one directory below.
          names.push_back(basename);
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::normalize_mount_point
//       Access: Private
//  Description: Converts the mount point string supplied by the user
//               to standard form (relative to the current directory,
//               with no double slashes, and not terminating with a
//               slash).  The initial slash is removed.
////////////////////////////////////////////////////////////////////
Filename VirtualFileSystem::
normalize_mount_point(const string &mount_point) const {
  Filename nmp = mount_point;
  if (nmp.is_local()) {
    nmp = Filename(_cwd, mount_point);
  }
  nmp.standardize();
  nassertr(!nmp.empty() && nmp[0] == '/', nmp);
  return nmp.get_fullpath().substr(1);
}

////////////////////////////////////////////////////////////////////
//     Function: VirtualFileSystem::found_match
//       Access: Private
//  Description: Evaluates one match found during a get_file()
//               operation.  There may be multiple matches for a
//               particular filename due to the ambiguities introduced
//               by allowing multiple mount points, so we may have to
//               keep searching even after the first match is found.
//
//               Returns true if the search should terminate now, or
//               false if it should keep iterating.
////////////////////////////////////////////////////////////////////
bool VirtualFileSystem::
found_match(PT(VirtualFile) &found_file, VirtualFileComposite *&composite_file,
            VirtualFileMount *mount, const string &local_filename) const {
  if (found_file == (VirtualFile *)NULL) {
    // This was our first match.  Save it.
    found_file = new VirtualFileSimple(mount, local_filename);
    if (!mount->is_directory(local_filename)) {
      // If it's not a directory, we're done.
      return true;
    }
    
  } else {
    // This was our second match.  The previous match(es) must
    // have been directories.
    if (!mount->is_directory(local_filename)) {
      // However, this one isn't a directory.  We're done.
      return true;
    }

    // At least two directories matched to the same path.  We
    // need a composite directory.
    if (composite_file == (VirtualFileComposite *)NULL) {
      composite_file =
        new VirtualFileComposite((VirtualFileSystem *)this, found_file->get_filename());
      composite_file->add_component(found_file);
      found_file = composite_file;
    }
    composite_file->add_component(new VirtualFileSimple(mount, local_filename));
  }

  // Keep going, looking for more directories.
  return false;
}

