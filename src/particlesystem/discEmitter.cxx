// Filename: discEmitter.cxx
// Created by:  charles (22Jun00)
//
////////////////////////////////////////////////////////////////////

#include "discEmitter.h"

////////////////////////////////////////////////////////////////////
//    Function : DiscEmitter::DiscEmitter
//      Access : Public
// Description : constructor
////////////////////////////////////////////////////////////////////
DiscEmitter::
DiscEmitter(void) {
  _radius = 1.0f;
  _inner_aoe = _outer_aoe = 0.0f;
  _inner_magnitude = _outer_magnitude = 1.0f;
  _cubic_lerping = false;
}

////////////////////////////////////////////////////////////////////
//    Function : DiscEmitter::DiscEmitter
//      Access : Public
// Description : copy constructor
////////////////////////////////////////////////////////////////////
DiscEmitter::
DiscEmitter(const DiscEmitter &copy) :
  BaseParticleEmitter(copy) {
  _radius = copy._radius;
  _inner_aoe = copy._inner_aoe;
  _outer_aoe = copy._outer_aoe;
  _inner_magnitude = copy._inner_magnitude;
  _outer_magnitude = copy._outer_magnitude;
  _cubic_lerping = copy._cubic_lerping;

  _distance_from_center = copy._distance_from_center;
  _sinf_theta = copy._sinf_theta;
  _cosf_theta = copy._cosf_theta;
}

////////////////////////////////////////////////////////////////////
//    Function : DiscEmitter::~DiscEmitter
//      Access : Public
// Description : destructor
////////////////////////////////////////////////////////////////////
DiscEmitter::
~DiscEmitter(void) {
}

////////////////////////////////////////////////////////////////////
//    Function : make_copy
//      Access : Public
// Description : copier
////////////////////////////////////////////////////////////////////
BaseParticleEmitter *DiscEmitter::
make_copy(void) {
  return new DiscEmitter(*this);
}

////////////////////////////////////////////////////////////////////
//    Function : DiscEmitter::assign_initial_position
//      Access : Public
// Description : Generates a location for a new particle
////////////////////////////////////////////////////////////////////
void DiscEmitter::
assign_initial_position(LPoint3f& pos) {
  // position
  float theta = NORMALIZED_RAND() * 2.0f * MathNumbers::pi_f;

  _distance_from_center = NORMALIZED_RAND();
  float r_scalar = _distance_from_center * _radius;

  _sinf_theta = sinf(theta);
  _cosf_theta = cosf(theta);

  float new_x = _cosf_theta * r_scalar;
  float new_y = _sinf_theta * r_scalar;

  pos.set(new_x, new_y, 0.0f);
}

////////////////////////////////////////////////////////////////////
//    Function : DiscEmitter::assign_initial_velocity
//      Access : Public
// Description : Generates a velocity for a new particle
////////////////////////////////////////////////////////////////////
void DiscEmitter::
assign_initial_velocity(LVector3f& vel) {
  float aoe, mag;

  // lerp type
  if (_cubic_lerping == true) {
    aoe = CLERP(_distance_from_center, _inner_aoe, _outer_aoe);
    mag = CLERP(_distance_from_center, _inner_magnitude, _outer_magnitude);
  }
  else {
    aoe = LERP(_distance_from_center, _inner_aoe, _outer_aoe);
    mag = LERP(_distance_from_center, _inner_magnitude, _outer_magnitude);
  }

  // velocity
  float vel_z = mag * sinf(deg_2_rad(aoe));
  float abs_diff = fabs((mag * mag) - (vel_z * vel_z));
  float root_mag_minus_z_squared = sqrtf(abs_diff);
  float vel_x = _cosf_theta * root_mag_minus_z_squared;
  float vel_y = _sinf_theta * root_mag_minus_z_squared;

  // quick and dirty
  if((aoe > 90.0f) && (aoe < 270.0f))
  {
    vel_x = -vel_x;
    vel_y = -vel_y;
  }

  vel.set(vel_x, vel_y, vel_z);
}
