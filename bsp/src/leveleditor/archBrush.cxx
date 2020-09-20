#include "archBrush.h"
#include "deg_2_rad.h"
#include "editorUtils.h"
#include "solid.h"
#include "solidFace.h"
#include "solidCollection.h"

bool CArchBrush::
create(const LPoint3 &mins, const LPoint3 &maxs, CMaterialReference *material,
       int round_decimals, int num_sides, int wall_width, int arc, int start_angle,
       int add_height, bool curved_ramp, int tilt_angle, bool tilt_interp,
       CSolidCollection &solids) {

  if (num_sides < 3) {
    return false;
  }

  if (wall_width < 1) {
    return false;
  }

  if (arc < 1) {
    return false;
  }

  if (start_angle < 0 || start_angle > 359) {
    return false;
  }

  if (std::abs(tilt_angle % 180) == 90) {
    return false;
  }

  // Very similar to the pipe brush, except with options for start angle, arc, height and tilt.
  PN_stdfloat width = maxs[0] - mins[0];
  PN_stdfloat length = maxs[1] - mins[1];
  PN_stdfloat height = maxs[2] - mins[2];

  PN_stdfloat major_out = width / 2.0;
  PN_stdfloat major_in = major_out - wall_width;
  PN_stdfloat minor_out = length / 2.0;
  PN_stdfloat minor_in = minor_out - wall_width;

  PN_stdfloat start = deg_2_rad((PN_stdfloat)start_angle);
  PN_stdfloat tilt = deg_2_rad((PN_stdfloat)tilt_angle);
  PN_stdfloat angle = deg_2_rad((PN_stdfloat)arc) / num_sides;

  LPoint3 center = (mins + maxs) / 2.0;

  // Calculate the coordinates of the inner and outer ellipses points.
  pvector<LPoint3> outer;
  pvector<LPoint3> inner;
  outer.reserve(num_sides + 1);
  inner.reserve(num_sides + 1);
  for (int i = 0; i < num_sides + 1; i++) {
    PN_stdfloat a = start + i * angle;
    PN_stdfloat h = i * add_height;
    PN_stdfloat interp = 1.0;
    if (tilt_interp) {
      interp = std::cos(M_PI / num_sides * (i - num_sides / 2.0));
    }
    PN_stdfloat tilt_height = wall_width / 2.0 * interp * std::tan(tilt);

    LPoint3 val(center[0] + major_out * std::cos(a),
                center[1] + minor_out * std::sin(a),
                mins[2]);
    if (curved_ramp) {
      val[2] += h + tilt_height;
    }
    outer.push_back(EditorUtils::round_vector(val, round_decimals));

    val = LPoint3(center[0] + major_in * std::cos(a),
                  center[1] + minor_in * std::sin(a),
                  mins[2]);
    if (curved_ramp) {
      val[2] += h - tilt_height;
    }
    inner.push_back(EditorUtils::round_vector(val, round_decimals));
  }

  // Create the solids
  LPoint3 z = EditorUtils::round_vector(LPoint3(0, 0, height), round_decimals);
  for (int i = 0; i < num_sides; i++) {

    // Since we are triangulating/splitting each arch segment, we need to generate 2 brushes per side
    if (curved_ramp) {

      PT(CSolid) solid1 = new CSolid;
      // The splitting orientation depends on the curving direction of the arch
      if (add_height >= 0) {
        solid1->add_face(new CSolidFace({ outer[i],     outer[i] + z,   outer[i+1] + z, outer[i+1] }, material, solid1));
        solid1->add_face(new CSolidFace({ outer[i+1],   outer[i+1] + z, inner[i] + z,   inner[1]},    material, solid1));
        solid1->add_face(new CSolidFace({ inner[i],     inner[i] + z,   outer[i] + z,   outer[i] },   material, solid1));
        solid1->add_face(new CSolidFace({ outer[i] + z, inner[i] + z,   outer[i+1] + z },             material, solid1));
        solid1->add_face(new CSolidFace({ outer[i+1],   inner[i],       outer[i] },                   material, solid1));

      } else {
        solid1->add_face(new CSolidFace({ inner[i+1],     inner[i+1] + z, inner[i] + z,   inner[i] },   material, solid1));
        solid1->add_face(new CSolidFace({ outer[i],       outer[i] + z,   inner[i+1] + z, inner[i+1] }, material, solid1));
        solid1->add_face(new CSolidFace({ inner[i],       inner[i] + z,   outer[i] + z,   outer[i] },   material, solid1));
        solid1->add_face(new CSolidFace({ inner[i+1] + z, outer[i] + z,   inner[i] + z },               material, solid1));
        solid1->add_face(new CSolidFace({ inner[i],       outer[i],       inner[i+1] },                 material, solid1));
      }

      solid1->generate_faces();
      solids.add_solid(solid1);

      PT(CSolid) solid2 = new CSolid;
      if (add_height >= 0) {
        solid2->add_face(new CSolidFace({ inner[i+1],      inner[i+1] + z, inner[i] + z,   inner[i] }, material, solid2));
        solid2->add_face(new CSolidFace({ inner[i],        inner[i] + z,   outer[i+1] + z, outer[i+1] }, material, solid2));
        solid2->add_face(new CSolidFace({ outer[i+1],      outer[i+1] + z, inner[i+1] + z, inner[i+1] }, material, solid2));
        solid2->add_face(new CSolidFace({ inner[i+1] + z,  outer[i+1] + z, inner[i] + z }, material, solid2));
        solid2->add_face(new CSolidFace({ inner[i],        outer[i+1],     inner[i+1] }, material, solid2));

      } else {
        solid2->add_face(new CSolidFace({ outer[i],        outer[i] + z,   outer[i+1] + z, outer[i+1] }, material, solid2));
        solid2->add_face(new CSolidFace({ inner[i+1],      inner[i+1] + z, outer[i] + z,   outer[i] }, material, solid2));
        solid2->add_face(new CSolidFace({ outer[i+1],      outer[i+1] + z, inner[i+1] + z, inner[i+1] }, material, solid2));
        solid2->add_face(new CSolidFace({ outer[i] + z,    inner[i+1] + z, outer[i+1] + z }, material, solid2));
        solid2->add_face(new CSolidFace({ outer[i+1],      inner[i+1],     outer[i] }, material, solid2));
      }

      solid2->generate_faces();
      solids.add_solid(solid2);

    } else {
      PT(CSolid) solid = new CSolid;
      LVector3 h = LVector3::unit_z() * i * add_height;
      solid->add_face(new CSolidFace({ outer[i] + h,    outer[i] + z + h,   outer[i+1] + z + h, outer[i+1] + h }, material, solid));
      solid->add_face(new CSolidFace({ inner[i+1] + h, inner[i+1] + z + h, inner[i] + z + h, inner[i] + h }, material, solid));
      solid->add_face(new CSolidFace({ outer[i+1] + h, outer[i+1] + z + h, inner[i+1] + z + h, inner[i+1] + h }, material, solid));
      solid->add_face(new CSolidFace({ inner[i] + h, inner[i] + z + h, outer[i] + z + h, outer[i] + h }, material, solid));
      solid->add_face(new CSolidFace({ inner[i+1] + z + h, outer[i+1] + z + h, outer[i] + z + h, inner[i] + z + h }, material, solid));
      solid->add_face(new CSolidFace({ inner[i] + h, outer[i] + h, outer[i+1] + h, inner[i+1] + h }, material, solid));

      solid->generate_faces();
      solids.add_solid(solid);
    }
  }

  return true;
}
