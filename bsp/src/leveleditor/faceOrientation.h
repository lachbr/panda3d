#ifndef FACEORIENTATION_H
#define FACEORIENTATION_H

#include "config_leveleditor.h"
#include "luse.h"

BEGIN_PUBLISH

enum class Align {
  Left,
  Right,
  Center,
  Top,
  Bottom,
};

enum class FaceOrientation {
  Floor,
  Ceiling,
  NorthWall,
  SouthWall,
  EastWall,
  WestWall,
  Invalid,
};

extern EXPCL_EDITOR LVector3 FaceNormals[6];
extern EXPCL_EDITOR LVector3 DownVectors[6];
extern EXPCL_EDITOR LVector3 RightVectors[6];

END_PUBLISH

#endif // FACEORIENTATION_H
