// Filename: materialPool.cxx
// Created by:  drose (30Apr01)
// 
////////////////////////////////////////////////////////////////////

#include "materialPool.h"
#include "config_gobj.h"


MaterialPool *MaterialPool::_global_ptr = (MaterialPool *)NULL;


////////////////////////////////////////////////////////////////////
//     Function: MaterialPool::ns_get_material
//       Access: Public
//  Description: The nonstatic implementation of get_material().
////////////////////////////////////////////////////////////////////
const Material *MaterialPool::
ns_get_material(const CPT(Material) &temp) {
  Materials::iterator mi = _materials.insert(temp).first;
  return (*mi);
}

////////////////////////////////////////////////////////////////////
//     Function: MaterialPool::ns_garbage_collect
//       Access: Private
//  Description: The nonstatic implementation of garbage_collect().
////////////////////////////////////////////////////////////////////
int MaterialPool::
ns_garbage_collect() {
  int num_released = 0;
  Materials new_set;

  Materials::iterator mi;
  for (mi = _materials.begin(); mi != _materials.end(); ++mi) {
    const Material *mat = (*mi);
    if (mat->get_ref_count() == 1) {
      if (gobj_cat.is_debug()) {
	gobj_cat.debug()
	  << "Releasing " << *mat << "\n";
      }
      num_released++;
    } else {
      new_set.insert(new_set.end(), *mi);
    }
  }

  _materials.swap(new_set);
  return num_released;
}

////////////////////////////////////////////////////////////////////
//     Function: MaterialPool::ns_list_contents
//       Access: Private
//  Description: The nonstatic implementation of list_contents().
////////////////////////////////////////////////////////////////////
void MaterialPool::
ns_list_contents(ostream &out) {
  out << _materials.size() << " materials:\n";
  Materials::iterator mi;
  for (mi = _materials.begin(); mi != _materials.end(); ++mi) {
    const Material *mat = (*mi);
    out << "  " << *mat
	<< " (count = " << mat->get_ref_count() << ")\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MaterialPool::get_ptr
//       Access: Private, Static
//  Description: Initializes and/or returns the global pointer to the
//               one MaterialPool object in the system.
////////////////////////////////////////////////////////////////////
MaterialPool *MaterialPool::
get_ptr() {
  if (_global_ptr == (MaterialPool *)NULL) {
    _global_ptr = new MaterialPool;
  }
  return _global_ptr;
}
