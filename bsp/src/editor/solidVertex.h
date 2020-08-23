#pragma once

#include "config_editor.h"
#include "dtoolbase.h"
#include "luse.h"
#include "geomVertexFormat.h"

class CSolid;

/**
 * A single vertex of a solid face.
 */
class EXPCL_EDITOR CSolidVertex {
PUBLISHED:
  CSolidVertex();
  LPoint3 get_world_pos(const CSolid *solid) const;
  void xform(const LMatrix4 &mat);

  LPoint3 pos;
  LNormal normal;
  LVector3 tangent;
  LVector3 binormal;
  LTexCoord texcoord;
  LTexCoord lightmap_texcoord;
};

INLINE CSolidVertex::
CSolidVertex() {
  normal.set(0, 1, 0);
  tangent.set(1, 0, 0);
  binormal.set(0, 0, 1);
}

INLINE void CSolidVertex::
xform(const LMatrix4 &mat) {
  pos = mat.xform_point(pos);
}
