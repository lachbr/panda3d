// Filename: interrogateManifest.C
// Created by:  drose (11Aug00)
// 
////////////////////////////////////////////////////////////////////

#include "interrogateManifest.h"
#include "indexRemapper.h"
#include "interrogate_datafile.h"

////////////////////////////////////////////////////////////////////
//     Function: InterrogateManifest::output
//       Access: Public
//  Description: Formats the InterrogateManifest data for output to a data
//               file.
////////////////////////////////////////////////////////////////////
void InterrogateManifest::
output(ostream &out) const {
  InterrogateComponent::output(out);
  out << _flags << " "
      << _int_value << " "
      << _type << " "
      << _getter << " ";
  idf_output_string(out, _definition);
}

////////////////////////////////////////////////////////////////////
//     Function: InterrogateManifest::input
//       Access: Public
//  Description: Reads the data file as previously formatted by
//               output().
////////////////////////////////////////////////////////////////////
void InterrogateManifest::
input(istream &in) {
  InterrogateComponent::input(in);
  in >> _flags >> _int_value >> _type >> _getter;
  idf_input_string(in, _definition);
}

////////////////////////////////////////////////////////////////////
//     Function: InterrogateManifest::remap_indices
//       Access: Public
//  Description: Remaps all internal index numbers according to the
//               indicated map.  This called from
//               InterrogateDatabase::remap_indices().
////////////////////////////////////////////////////////////////////
void InterrogateManifest::
remap_indices(const IndexRemapper &remap) {
  _type = remap.map_from(_type);
  _getter = remap.map_from(_getter);
}
