#include "solidVertex.h"
#include "solid.h"

/**
 * Returns the world space vertex position, given that the vertex
 * is in the coordinate frame of `face`.
 */
LPoint3 CSolidVertex::
get_world_pos(const CSolid *solid) const {
  LMatrix4 mat = solid->get_np().get_mat(NodePath());
  if (mat.is_identity() || mat.is_nan()) {
    return pos;
  }

  return mat.xform_point(pos);
}
