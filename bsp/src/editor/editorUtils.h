#pragma once

#include "config_editor.h"
#include "luse.h"

class EXPCL_EDITOR EditorUtils {
PUBLISHED:
  template<class T>
  static T round_vector(T vec, int num = 8);

  static LVecBase2f round_vec2(LVecBase2f vec, int num = 8);
  static LVecBase3f round_vec3(LVecBase3f vec, int num = 8);
  static LVecBase4f round_vec4(LVecBase4f vec, int num = 8);

  static PN_stdfloat round_up(PN_stdfloat val, int num);
};

INLINE PN_stdfloat EditorUtils::
round_up(PN_stdfloat val, int num) {
  const PN_stdfloat mult = std::pow(10, num);
  return std::round(val * mult) / mult;
}

template<class T>
INLINE T EditorUtils::
round_vector(T vec, int num) {
  size_t count = vec.size();
  for (size_t i = 0; i < count; i++) {
    vec[i] = round_up(vec[i], num);
  }

  return vec;
}

INLINE LVecBase2f EditorUtils::
round_vec2(LVecBase2f vec, int num) {
  return round_vector(vec, num);
}

INLINE LVecBase3f EditorUtils::
round_vec3(LVecBase3f vec, int num) {
  return round_vector(vec, num);
}

INLINE LVecBase4f EditorUtils::
round_vec4(LVecBase4f vec, int num) {
  return round_vector(vec, num);
}
