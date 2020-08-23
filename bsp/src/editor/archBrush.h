#pragma once

#include "config_editor.h"
#include "baseBrush.h"
#include "luse.h"

class CSolidCollection;
class CMaterialReference;

class EXPCL_EDITOR CArchBrush : public CBaseBrush {
PUBLISHED:
  bool create(const LPoint3 &mins, const LPoint3 &maxs, CMaterialReference *material,
              int round_decimals, int num_sides, int wall_width, int arc, int start_angle,
              int add_height, bool curved_ramp, int tilt_angle, bool tilt_interp,
              CSolidCollection &solids);
};
