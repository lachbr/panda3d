// Filename: cvsCopy.h
// Created by:  drose (31Oct00)
// 
////////////////////////////////////////////////////////////////////

#ifndef CVSCOPY_H
#define CVSCOPY_H

#include <pandatoolbase.h>

#include "cvsSourceTree.h"

#include <programBase.h>
#include <filename.h>

////////////////////////////////////////////////////////////////////
// 	 Class : CVSCopy
// Description : This is the base class for a family of programs that
//               copy files, typically model files like .flt files and
//               their associated textures, into a CVS-controlled
//               source tree.
////////////////////////////////////////////////////////////////////
class CVSCopy : public ProgramBase {
public:
  CVSCopy();

  CVSSourceDirectory *
  import(const Filename &source, void *extra_data, 
	 CVSSourceDirectory *suggested_dir);

protected:
  virtual bool handle_args(Args &args);
  virtual bool post_command_line();

  virtual bool copy_file(const Filename &source, const Filename &dest,
			 CVSSourceDirectory *dest_dir,
			 void *extra_data, bool new_file)=0;

  bool copy_binary_file(Filename source, Filename dest);

  bool create_file(const Filename &filename);

private:
  bool scan_hierarchy(); 
  bool scan_for_root(const string &dirname);
  
protected:
  bool _force;
  bool _interactive;
  bool _got_model_dirname;
  Filename _model_dirname;
  bool _got_map_dirname;
  Filename _map_dirname;
  bool _got_root_dirname;
  Filename _root_dirname;
  Filename _key_filename;
  bool _no_cvs;
  string _cvs_binary;

  typedef vector_string SourceFiles;
  SourceFiles _source_files;

  CVSSourceTree _tree;
  CVSSourceDirectory *_model_dir;
  CVSSourceDirectory *_map_dir;

  typedef map<string, CVSSourceDirectory *> CopiedFiles;
  CopiedFiles _copied_files;
};

#endif
