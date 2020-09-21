#ifndef RAYTRACEHITRESULT4_H
#define RAYTRACEHITRESULT4_H

#ifndef CPPPARSER

#include "config_raytrace.h"
#include "mathlib/ssemath.h"

class ALIGN_16BYTE EXPCL_BSP_RAYTRACE RayTraceHitResult4
{
public:
        FourVectors hit_normal;
        FourVectors hit_uv;
        u32x4 prim_id;
        u32x4 geom_id;
        fltx4 hit_fraction;
        u32x4 hit;
};
#endif

#endif // RAYTRACEHITRESULT4_h
