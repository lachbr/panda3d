// Filename: stitchCommand.cxx
// Created by:  drose (08Nov99)
// 
////////////////////////////////////////////////////////////////////

#include "stitchCommand.h"
#include "stitchImage.h"
#include "stitchLens.h"
#include "stitchPerspectiveLens.h"
#include "stitchFisheyeLens.h"
#include "stitchCylindricalLens.h"
#include "stitchPSphereLens.h"
#include "stitchImageOutputter.h"
#include "stitcher.h"

#include <indent.h>
#include <pnmImage.h>

ostream &
operator << (ostream &out, StitchCommand::Command c) {
  switch (c) {
  case StitchCommand::C_global:
    return out << "global";
    break;

  case StitchCommand::C_define:
    return out << "define";
    break;

  case StitchCommand::C_lens:
    return out << "lens";
    break;

  case StitchCommand::C_input_image:
    return out << "input_image";
    break;

  case StitchCommand::C_output_image:
    return out << "output_image";
    break;

  case StitchCommand::C_perspective:
    return out << "perspective";
    break;

  case StitchCommand::C_fisheye:
    return out << "fisheye";
    break;

  case StitchCommand::C_cylindrical:
    return out << "cylindrical";
    break;

  case StitchCommand::C_psphere:
    return out << "psphere";
    break;

  case StitchCommand::C_focal_length:
    return out << "focal_length";
    break;

  case StitchCommand::C_fov:
    return out << "fov";
    break;

  case StitchCommand::C_singularity_tolerance:
    return out << "singularity_tolerance";
    break;

  case StitchCommand::C_resolution:
    return out << "resolution";
    break;

  case StitchCommand::C_filename:
    return out << "filename";
    break;

  case StitchCommand::C_point2d:
  case StitchCommand::C_point3d:
    return out << "point";
    break;

  case StitchCommand::C_show_points:
    return out << "show_points";
    break;

  case StitchCommand::C_image_size:
    return out << "image_size";
    break;

  case StitchCommand::C_film_size:
    return out << "film_size";
    break;

  case StitchCommand::C_grid:
    return out << "grid";
    break;

  case StitchCommand::C_untextured_color:
    return out << "untextured_color";
    break;

  case StitchCommand::C_hpr:
    return out << "hpr";
    break;

  case StitchCommand::C_layers:
    return out << "layers";
    break;

  case StitchCommand::C_stitch:
    return out << "stitch";
    break;

  case StitchCommand::C_using:
    return out << "using";
    break;

  case StitchCommand::C_user_command:
    return out << "user_command";
    break;
   
  default:
    return out << "(**unknown command**)";
  }
}

StitchCommand::
StitchCommand(StitchCommand *parent, StitchCommand::Command command) :
  _parent(parent),
  _command(command)
{
  _params = 0;
  _lens = NULL;
  if (parent != NULL) {
    parent->add_nested(this);
  }
}

StitchCommand::
~StitchCommand() {
  Commands::iterator ci;
  for (ci = _nested.begin(); ci != _nested.end(); ++ci) {
    delete (*ci);
  }
}

void StitchCommand::
clear() {
  Commands::iterator ci;
  for (ci = _nested.begin(); ci != _nested.end(); ++ci) {
    delete (*ci);
  }
  _nested.clear();
  _using.clear();
  _params = 0;
  _command = C_global;
}

void StitchCommand::
set_name(const string &name) {
  if (!name.empty()) {
    _params |= P_name;
    _name = name;
  }
}

void StitchCommand::
set_length(double number) {
  _params |= P_length;
  _number = number;
}

void StitchCommand::
set_resolution(double number) {
  _params |= P_resolution;
  _number = number;
}

void StitchCommand::
set_number(double number) {
  _params |= P_number;
  _number = number;
}

void StitchCommand::
set_point2d(const LVecBase2d &point) {
  _params |= P_point2d;
  _n[0] = point[0];
  _n[1] = point[1];
}

void StitchCommand::
set_point3d(const LVecBase3d &point) {
  _params |= P_point3d;
  _n[0] = point[0];
  _n[1] = point[1];
  _n[2] = point[2];
}

void StitchCommand::
set_length_pair(const LVecBase2d &length_pair) {
  _params |= P_length_pair;
  _n[0] = length_pair[0];
  _n[1] = length_pair[1];
}

void StitchCommand::
set_color(const Colord &color) {
  _params |= P_color;
  _n[0] = color[0];
  _n[1] = color[1];
  _n[2] = color[2];
  _n[3] = color[3];
}

void StitchCommand::
set_str(const string &str) {
  _params |= P_str;
  _str = str;
}

bool StitchCommand::
add_using(const string &name) {
  StitchCommand *def = find_definition(name);
  if (def != NULL) {
    _params |= P_using;
    _using.push_back(def);
    return true;
  }
  return false;
}

void StitchCommand::
add_nested(StitchCommand *nested) {
  _params |= P_nested;
  _nested.push_back(nested);
}

string StitchCommand::
get_name() const {
  return _name;
}

double StitchCommand::
get_number() const {
  return _number;
}

LVecBase2d StitchCommand::
get_point2d() const {
  return LVecBase2d(_n[0], _n[1]);
}

LVecBase3d StitchCommand::
get_point3d() const {
  return LVecBase3d(_n[0], _n[1], _n[2]);
}

LVector3d StitchCommand::
get_vector3d() const {
  return LVector3d(_n[0], _n[1], _n[2]);
}

Colord StitchCommand::
get_color() const {
  return Colord(_n[0], _n[1], _n[2], _n[3]);
}

string StitchCommand::
get_str() const {
  return _str;
}

void StitchCommand::
process(StitchImageOutputter &outputter, Stitcher *stitcher,
	StitchFile &file) {
  if (_command == C_input_image) {
    StitchImage *image = create_image();

    if (stitcher != NULL) {
      stitcher->add_image(image);
    } else {
      outputter.add_input_image(image);
    }

  } else if (_command == C_output_image) {
    StitchImage *image = create_image();
    outputter.add_output_image(image);

  } else if (_command == C_stitch) {
    Stitcher *new_stitcher = new Stitcher;
    Commands::const_iterator ci;
    for (ci = _nested.begin(); ci != _nested.end(); ++ci) {
      (*ci)->process(outputter, new_stitcher, file);
    }
    new_stitcher->stitch();

    // Now add all of the stitched images to the outputter, in order.
    Stitcher::Images::const_iterator ii;
    for (ii = new_stitcher->_placed.begin();
	 ii != new_stitcher->_placed.end();
	 ++ii) {
      outputter.add_input_image(*ii);
    }
    outputter.add_stitcher(new_stitcher);

  } else if (_command == C_point3d) {
    if (stitcher != NULL) {
      stitcher->add_point(_name, get_vector3d());
    }

  } else if (_command == C_show_points) {
    if (stitcher != NULL) {
      stitcher->show_points(get_number(), get_color());
    }

  } else if (_params & P_nested) {
    Commands::const_iterator ci;
    for (ci = _nested.begin(); ci != _nested.end(); ++ci) {
      (*ci)->process(outputter, stitcher, file);
    }
  }
}

void StitchCommand::
write(ostream &out, int indent_level) const {
  if (_command == C_user_command) {
    assert(_using.size() == 1);
    indent(out, indent_level) << _using.front()->_name << ";\n";

  } else if (_command == C_global) {
    Commands::const_iterator ci;
    for (ci = _nested.begin(); ci != _nested.end(); ++ci) {
      (*ci)->write(out, indent_level );
    }

  } else {
    indent(out, indent_level) << _command;
    if (_params & P_name) {
      out << " " << _name;
    }
    if (_params & P_length) {
      out << " " << get_number() << "mm";
    }
    if (_params & P_resolution) {
      out << " " << get_number() << "p/mm";
    }
    if (_params & P_number) {
      out << " " << get_number();
    }
    if (_params & P_point2d) {
      out << " (" << get_point2d() << ")";
    }
    if (_params & P_point3d) {
      out << " (" << get_point3d() << ")";
    }
    if (_params & P_length_pair) {
      out << " (" << _n[0] << "mm " << _n[1] << "mm)";
    }
    if (_params & P_color) {
      out << " (" << get_color() << ")";
    }
    if (_params & P_str) {
      out << " \"" << _str << "\"";
    }
    if (_params & P_using) {
      Commands::const_iterator ci;
      ci = _using.begin();
      if (ci != _using.end()) {
	out << " " << (*ci)->_name;
	++ci;
	while (ci != _using.end()) {
	  out << ", " << (*ci)->_name;
	  ++ci;
	}
      }
    }
    if (_params & P_nested) {
      out << " {\n";
      Commands::const_iterator ci;
      for (ci = _nested.begin(); ci != _nested.end(); ++ci) {
	(*ci)->write(out, indent_level + 2);
      }
      indent(out, indent_level) << "}\n";
    } else {
      out << ";\n";
    }
  }
}



StitchCommand *StitchCommand::
find_definition(const string &name) {
  Commands::const_iterator ci;
  for (ci = _nested.begin(); ci != _nested.end(); ++ci) {
    if (((*ci)->_command == C_define || (*ci)->_command == C_lens) && 
	(*ci)->_name == name) {
      return (*ci);
    }
  }
  if (_parent != NULL) {
    return _parent->find_definition(name);
  }
  return NULL;
}

StitchLens *StitchCommand::
find_using_lens() {
  if (!_using.empty()) {
    Commands::const_iterator ci;
    for (ci = _using.begin(); ci != _using.end(); ++ci) {
      StitchLens *lens = (*ci)->find_lens();
      if (lens != NULL) {
	return lens;
      }
    }
  }
  if (_parent != NULL) {
    return _parent->find_using_lens();
  }
  return NULL;
}

StitchLens *StitchCommand::
find_lens() {
  if (_command == C_lens) {
    return make_lens();
  }
  Commands::const_iterator ci;
  for (ci = _nested.begin(); ci != _nested.end(); ++ci) {
    if ((*ci)->_command == C_lens) {
      return (*ci)->make_lens();
    }    
  }
  if (_parent != NULL) {
    return _parent->find_using_lens();
  }
  return NULL;
}

StitchLens *StitchCommand::
make_lens() {
  if (_lens != NULL) {
    return _lens;
  }

  if (find_command(C_fisheye) != NULL) {
    _lens = new StitchFisheyeLens();
  } else if (find_command(C_cylindrical) != NULL) {
    _lens = new StitchCylindricalLens();
  } else if (find_command(C_psphere) != NULL) {
    _lens = new StitchPSphereLens();
  } else {
    _lens = new StitchPerspectiveLens();
  }

  StitchCommand *cmd = find_command(C_focal_length);
  if (cmd != NULL) {
    _lens->set_focal_length(cmd->get_number());
  }
  cmd = find_command(C_fov);
  if (cmd != NULL) {
    _lens->set_hfov(cmd->get_number());
  }

  if (!_lens->is_defined()) {
    _lens->set_hfov(60.0);
  }

  Commands::const_iterator ci;
  for (ci = _nested.begin(); ci != _nested.end(); ++ci) {
    switch ((*ci)->_command) {
    case C_singularity_tolerance:
      _lens->set_singularity_tolerance((*ci)->get_number());
      break;

    default:
      break;
    }
  }

  return _lens;
}


StitchCommand *StitchCommand::
find_using_command(Command command) {
  if (!_using.empty()) {
    Commands::const_iterator ci;
    for (ci = _using.begin(); ci != _using.end(); ++ci) {
      StitchCommand *cmd = (*ci)->find_command(command);
      if (cmd != NULL) {
	return cmd;
      }
    }
  }

  if (_parent != NULL) {
    return _parent->find_using_command(command);
  }

  return NULL;
}

StitchCommand *StitchCommand::
find_command(Command command) {
  Commands::const_iterator ci;
  for (ci = _nested.begin(); ci != _nested.end(); ++ci) {
    if ((*ci)->_command == command) {
      return (*ci);
    }
  }

  if (_parent != NULL) {
    return _parent->find_using_command(command);
  }

  return NULL;
}

string StitchCommand::
find_parameter(Command command, const string &dflt) {
  StitchCommand *cmd = find_command(command);
  if (cmd != NULL) {
    return cmd->_str;
  } else {
    return dflt;
  }
}

double StitchCommand::
find_parameter(Command command, double dflt) {
  StitchCommand *cmd = find_command(command);
  if (cmd != NULL) {
    return cmd->get_number();
  } else {
    return dflt;
  }
}

LVecBase2d StitchCommand::
find_parameter(Command command, const LVecBase2d &dflt) {
  StitchCommand *cmd = find_command(command);
  if (cmd != NULL) {
    return cmd->get_point2d();
  } else {
    return dflt;
  }
}


StitchImage *StitchCommand::
create_image() {
  string filename = find_parameter(C_filename, "");
  LVecBase2d size_pixels(256, 256);
  LVecBase2d resolution(72.0 / 25.4, 72.0 / 25.4);
  StitchLens *lens = find_lens();
  if (lens == NULL) {
    nout << "Warning: No lens defined for " << filename << "\n";
    lens = make_lens();
  }
  
  StitchCommand *cmd;
  cmd = find_command(C_image_size);
  if (cmd != NULL) {
    size_pixels = cmd->get_point2d();

  } else if (!filename.empty()) {
    // If we don't get an explicit image size, try to determine it
    // from the image file.
    PNMImageHeader header;
    if (header.read_header(filename)) {
      size_pixels.set(header.get_x_size(), header.get_y_size());
    }
  }

  cmd = find_command(C_film_size);
  if (cmd != NULL) {
    LVecBase2d size_mm = cmd->get_point2d();
    resolution.set((size_pixels[0]-1) / size_mm[0],
		   (size_pixels[1]-1) / size_mm[1]);
  } else {
    cmd = find_command(C_resolution);
    if (cmd != NULL) {
      resolution.set(cmd->get_number(), cmd->get_number());
    }
  }

  StitchImage *image = 
    new StitchImage(get_name(), filename, lens, size_pixels, resolution);
  image->setup_grid(50, 50);

  // Also look for points and other stuff.
  Commands::const_iterator ci;
  for (ci = _nested.begin(); ci != _nested.end(); ++ci) {
    switch ((*ci)->_command) {
    case C_point2d:
      image->add_point((*ci)->_name, (*ci)->get_point2d());
      break;

    case C_show_points:
      image->show_points((*ci)->get_number(), (*ci)->get_color());
      break;

    case C_untextured_color:
      image->_untextured_color = (*ci)->get_color();
      break;

    case C_hpr:
      image->set_hpr((*ci)->get_point3d());
      break;

    case C_layers:
      image->_layered_type = StitchImage::LT_separate;
      break;

    case C_grid:
      image->setup_grid((int)(*ci)->_n[0], (int)(*ci)->_n[1]);
      break;

    default:
      break;
    }
  }

  return image;
}

