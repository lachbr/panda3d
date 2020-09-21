#include "faceOrientation.h"

LVector3 FaceNormals[6] = {
  LVector3(0, 0, 1),  // floor
  LVector3(0, 0, -1), // ceiling
  LVector3(0, -1, 0), // north wall
  LVector3(0, 1, 0),  // south wall
  LVector3(-1, 0, 0), // east wall
  LVector3(1, 0, 0),  // west wall
};

LVector3 DownVectors[6] = {
  LVector3(0, -1, 0),  // floor
  LVector3(0, -1, 0), // ceiling
  LVector3(0, 0, -1), // north wall
  LVector3(0, 0, -1),  // south wall
  LVector3(0, 0, -1), // east wall
  LVector3(0, 0, -1),  // west wall
};

LVector3 RightVectors[6] = {
  LVector3(1, 0, 0),  // floor
  LVector3(1, 0, 0), // ceiling
  LVector3(1, 0, 0), // north wall
  LVector3(1, 0, 0),  // south wall
  LVector3(0, 1, 0), // east wall
  LVector3(0, 1, 0),  // west wall
};
