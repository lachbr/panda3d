// Filename: somethingToEgg.cxx
// Created by:  drose (15Feb00)
// 
////////////////////////////////////////////////////////////////////

#include "somethingToEgg.h"

#include <config_util.h>

////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::Constructor
//       Access: Public
//  Description: The first parameter to the constructor should be the
//               one-word name of the file format that is to be read,
//               for instance "OpenFlight" or "Alias".  It's just used
//               in printing error messages and such.
////////////////////////////////////////////////////////////////////
SomethingToEgg::
SomethingToEgg(const string &format_name, 
	       const string &preferred_extension,
	       bool allow_last_param, bool allow_stdout) : 
  EggConverter(format_name, preferred_extension, allow_last_param, allow_stdout)
{
  clear_runlines();
  if (_allow_last_param) {
    add_runline("[opts] input" + _preferred_extension + " output.egg");
  }
  add_runline("[opts] -o output.egg input" + _preferred_extension);
  if (_allow_stdout) {
    add_runline("[opts] input" + _preferred_extension + " >output.egg");
  }

  // -f doesn't make sense if we aren't reading egg files.
  remove_option("f");

  redescribe_option
    ("cs",
     "Specify the coordinate system of the input " + _format_name +
     " file.  Normally, this can inferred from the file itself.");

  _input_units = DU_invalid;
  _output_units = DU_invalid;
  _texture_path_convert = SomethingToEggConverter::PC_unchanged;
  _model_path_convert = SomethingToEggConverter::PC_unchanged;
  _append_to_sys_paths = false;
  _merge_externals = false;
}

////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::add_units_options
//       Access: Public
//  Description: Adds -ui and -uo as valid options for this program.
//               If the user specifies -uo and -ui, or just -uo and
//               the program specifies -ui by setting _input_units,
//               the indicated units conversion will be automatically
//               applied before writing out the egg file.
////////////////////////////////////////////////////////////////////
void SomethingToEgg::
add_units_options() {
  add_option
    ("ui", "units", 40, 
     "Specify the units of the input " + _format_name +
     " file.  Normally, this can be inferred from the file itself.",
     &SomethingToEgg::dispatch_units, NULL, &_input_units);

  add_option
    ("uo", "units", 40, 
     "Specify the units of the resulting egg file.  If this is "
     "specified, the vertices in the egg file will be scaled as "
     "necessary to make the appropriate units conversion; otherwise, "
     "the vertices will be left as they are.",
     &SomethingToEgg::dispatch_units, NULL, &_output_units);
}

////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::add_texture_path_options
//       Access: Public
//  Description: Adds -rt etc. as valid options for this program.
//               These specify how the texture pathnames that appear
//               in the source file should be changed for the egg
//               file.
////////////////////////////////////////////////////////////////////
void SomethingToEgg::
add_texture_path_options() {
  add_option
    ("rt", "", 40,
     "Convert texture filenames to relative pathnames, relative to the "
     "directory specified by -rd.",
     &SomethingToEgg::dispatch_path_convert_relative, NULL, &_texture_path_convert);

  add_option
    ("rta", "", 40,
     "Convert texture filenames to absolute pathnames.",
     &SomethingToEgg::dispatch_path_convert_absolute, NULL, &_texture_path_convert);

  add_option
    ("rtA", "", 40,
     "Convert texture filenames to absolute pathnames that begin with the "
     "prefix specified by -rd.",
     &SomethingToEgg::dispatch_path_convert_rel_abs, NULL, &_texture_path_convert);

  add_option
    ("rts", "", 40,
     "Strip paths from texture filenames.  Only the basename of the texture "
     "will be preserved.",
     &SomethingToEgg::dispatch_path_convert_strip, NULL, &_texture_path_convert);

  add_option
    ("rtu", "", 40,
     "Leave texture filenames unchanged.  They will be relative if they "
     "are relative in the source file, or absolute if they are absolute in "
     "the source file.",
     &SomethingToEgg::dispatch_path_convert_unchanged, NULL, &_texture_path_convert);
}


////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::add_model_path_options
//       Access: Public
//  Description: Adds -re etc. as valid options for this program.
//               These specify how the model pathnames that appear
//               in the source file should be changed for the egg
//               file.
////////////////////////////////////////////////////////////////////
void SomethingToEgg::
add_model_path_options() {
  add_option
    ("re", "", 40,
     "Convert model filenames (external references) to relative pathnames, "
     "relative to the directory specified by -rd.",
     &SomethingToEgg::dispatch_path_convert_relative, NULL, &_model_path_convert);

  add_option
    ("rea", "", 40,
     "Convert model filenames to absolute pathnames.",
     &SomethingToEgg::dispatch_path_convert_absolute, NULL, &_model_path_convert);

  add_option
    ("reA", "", 40,
     "Convert model filenames to absolute pathnames that begin with the "
     "prefix specified by -rd.",
     &SomethingToEgg::dispatch_path_convert_rel_abs, NULL, &_model_path_convert);

  add_option
    ("res", "", 40,
     "Strip paths from model filenames.  Only the basename of the model "
     "will be preserved.",
     &SomethingToEgg::dispatch_path_convert_strip, NULL, &_model_path_convert);

  add_option
    ("reu", "", 40,
     "Leave model filenames unchanged.  They will be relative if they "
     "are relative in the source file, or absolute if they are absolute in "
     "the source file.",
     &SomethingToEgg::dispatch_path_convert_unchanged, NULL, &_model_path_convert);
}

////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::add_rel_dir_options
//       Access: Public
//  Description: Adds -rd.  This should generally be called if
//               add_texture_path_options() or
//               add_model_path_options() is called.
////////////////////////////////////////////////////////////////////
void SomethingToEgg::
add_rel_dir_options() {
  add_option
    ("rd", "dir", 40,
     "Specify the directory to make relative to.  This is the "
     "directory that all pathnames given in the source file will be "
     "rewritten to be relative to, if -re or -rt is given.  It is "
     "ignored if one of these options is not given.  If omitted, it "
     "is taken from the source filename.",
     &SomethingToEgg::dispatch_string, &_got_make_rel_dir, &_make_rel_dir);
}

////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::add_search_path_options
//       Access: Public
//  Description: Adds -rs.  If append_to_sys_paths is true, the
//               specified search path is prepended to the system
//               paths returned by get_texture_path() and
//               get_model_path(); otherwise, it's up to the caller to
//               do the right thing with the _search_path.
////////////////////////////////////////////////////////////////////
void SomethingToEgg::
add_search_path_options(bool append_to_sys_paths) {
  _append_to_sys_paths = append_to_sys_paths;
  add_option
    ("rs", "path", 40, 
     "A search path for textures and external file references.  This "
     "is a colon-separated set of directories that will be searched "
     "for filenames that are not fully specified in the source file.  It "
     "is unrelated to -re and -rt, and is used only if the source file "
     "does not store absolute pathnames.  The directory containing "
     "the source filename is always implicitly included.",
     &SomethingToEgg::dispatch_search_path, &_got_search_path, &_search_path);
}

////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::add_merge_externals_options
//       Access: Public
//  Description: Adds -f.
////////////////////////////////////////////////////////////////////
void SomethingToEgg::
add_merge_externals_options() {
  add_option
    ("f", "", 40, 
     "Follow and convert all external references in the source file.",
     &SomethingToEgg::dispatch_none, &_merge_externals);
}

////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::apply_units_scale
//       Access: Protected
//  Description: Applies the scale indicated by the input and output
//               units to the indicated egg file.  This is normally
//               done automatically when the file is written out.
////////////////////////////////////////////////////////////////////
void SomethingToEgg::
apply_units_scale(EggData &data) {
  if (_output_units != DU_invalid && _input_units != DU_invalid &&
      _input_units != _output_units) {
    nout << "Converting from " << format_long_unit(_input_units)
	 << " to " << format_long_unit(_output_units) << "\n";
    double scale = convert_units(_input_units, _output_units);
    data.transform(LMatrix4d::scale_mat(scale));
  }
}

////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::handle_args
//       Access: Protected
//  Description: 
////////////////////////////////////////////////////////////////////
bool SomethingToEgg::
handle_args(Args &args) {
  if (_allow_last_param && !_got_output_filename && args.size() > 1) {
    _got_output_filename = true;
    _output_filename = args.back();
    args.pop_back();

    if (!(_output_filename.get_extension() == "egg")) {
      nout << "Output filename " << _output_filename 
	   << " does not end in .egg.  If this is really what you intended, "
	"use the -o output_file syntax.\n";
      return false;
    }

    if (!verify_output_file_safe()) {
      return false;
    }
  }

  if (args.empty()) {
    nout << "You must specify the " << _format_name 
	  << " file to read on the command line.\n";
    return false;
  }

  if (args.size() != 1) {
    nout << "You may only specify one " << _format_name 
	 << " file to read on the command line.  "
	 << "You specified: ";
    copy(args.begin(), args.end(), ostream_iterator<string>(nout, " "));
    nout << "\n";
    return false;
  }

  _input_filename = args[0];

  if (!_input_filename.exists()) {
    nout << "Cannot find input file " << _input_filename << "\n";
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::post_command_line
//       Access: Protected, Virtual
//  Description: This is called after the command line has been
//               completely processed, and it gives the program a
//               chance to do some last-minute processing and
//               validation of the options and arguments.  It should
//               return true if everything is fine, false if there is
//               an error.
////////////////////////////////////////////////////////////////////
bool SomethingToEgg::
post_command_line() {
  if (!_got_make_rel_dir) {
    _make_rel_dir = _input_filename.get_dirname();
  }

  if (_append_to_sys_paths) {
    DSearchPath &texture_path = get_texture_path();
    DSearchPath &model_path = get_model_path();

    // Prepend the source filename to the search paths.
    Filename directory = _input_filename.get_dirname();
    if (directory.empty()) {
      directory = ".";
    }
    texture_path.prepend_directory(directory);
    model_path.prepend_directory(directory);

    // Then prepend whatever the user specified on the command line.
    if (_got_search_path) {
      texture_path.prepend_path(_search_path);
      model_path.prepend_path(_search_path);
    }
  }

  return EggConverter::post_command_line();
}

////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::post_process_egg_file
//       Access: Protected, Virtual
//  Description: Performs any processing of the egg file that is
//               appropriate before writing it out.  This includes any
//               normal adjustments the user requested via -np, etc.
//
//               Normally, you should not need to call this function
//               directly; write_egg_file() calls it for you.  You
//               should call this only if you do not use
//               write_egg_file() to write out the resulting egg file.
////////////////////////////////////////////////////////////////////
void SomethingToEgg::
post_process_egg_file() {
  apply_units_scale(_data);
  EggConverter::post_process_egg_file();
}

////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::dispatch_path_convert_relative
//       Access: Protected, Static
//  Description: Dispatch function to set the given path convert mode
//               to PC_relative.  var is a pointer to a
//               SomethingToEggConverter::PathConvert variable.
////////////////////////////////////////////////////////////////////
bool SomethingToEgg::
dispatch_path_convert_relative(const string &opt, const string &, void *var) {
  SomethingToEggConverter::PathConvert *ip = (SomethingToEggConverter::PathConvert *)var;
  (*ip) = SomethingToEggConverter::PC_relative;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::dispatch_path_convert_absolute
//       Access: Protected, Static
//  Description: Dispatch function to set the given path convert mode
//               to PC_absolute.  var is a pointer to a
//               SomethingToEggConverter::PathConvert variable.
////////////////////////////////////////////////////////////////////
bool SomethingToEgg::
dispatch_path_convert_absolute(const string &opt, const string &, void *var) {
  SomethingToEggConverter::PathConvert *ip = (SomethingToEggConverter::PathConvert *)var;
  (*ip) = SomethingToEggConverter::PC_absolute;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::dispatch_path_convert_rel_abs
//       Access: Protected, Static
//  Description: Dispatch function to set the given path convert mode
//               to PC_rel_abs.  var is a pointer to a
//               SomethingToEggConverter::PathConvert variable.
////////////////////////////////////////////////////////////////////
bool SomethingToEgg::
dispatch_path_convert_rel_abs(const string &opt, const string &, void *var) {
  SomethingToEggConverter::PathConvert *ip = (SomethingToEggConverter::PathConvert *)var;
  (*ip) = SomethingToEggConverter::PC_rel_abs;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::dispatch_path_convert_strip
//       Access: Protected, Static
//  Description: Dispatch function to set the given path convert mode
//               to PC_strip.  var is a pointer to a
//               SomethingToEggConverter::PathConvert variable.
////////////////////////////////////////////////////////////////////
bool SomethingToEgg::
dispatch_path_convert_strip(const string &opt, const string &, void *var) {
  SomethingToEggConverter::PathConvert *ip = (SomethingToEggConverter::PathConvert *)var;
  (*ip) = SomethingToEggConverter::PC_strip;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: SomethingToEgg::dispatch_path_convert_unchanged
//       Access: Protected, Static
//  Description: Dispatch function to set the given path convert mode
//               to PC_unchanged.  var is a pointer to a
//               SomethingToEggConverter::PathConvert variable.
////////////////////////////////////////////////////////////////////
bool SomethingToEgg::
dispatch_path_convert_unchanged(const string &opt, const string &, void *var) {
  SomethingToEggConverter::PathConvert *ip = (SomethingToEggConverter::PathConvert *)var;
  (*ip) = SomethingToEggConverter::PC_unchanged;
  return true;
}
