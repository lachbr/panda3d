#define OTHER_LIBS interrogatedb:c dconfig:c dtoolconfig:m \
                   dtoolutil:c dtoolbase:c dtool:m

#begin lib_target
  #define TARGET gobj
  #define LOCAL_LIBS \
    event linmath mathutil pnmimage gsgbase graph putil

  #define COMBINED_SOURCES $[TARGET]_composite1.cxx $[TARGET]_composite2.cxx 

  #define SOURCES \
     LOD.I LOD.h config_gobj.h drawable.h geom.I geom.N  \
     geom.h geomLine.h geomLinestrip.h geomPoint.h geomPolygon.h  \
     geomQuad.h geomSphere.h geomSprite.I geomSprite.h geomTri.h  \
     geomTrifan.h geomTristrip.h imageBuffer.I imageBuffer.h  \
     material.I material.h materialPool.I materialPool.h  \
     matrixLens.I matrixLens.h \
     orthographicLens.I orthographicLens.h perspectiveLens.I  \
     perspectiveLens.h pixelBuffer.I pixelBuffer.N  \
     pixelBuffer.h lens.h lens.I \
     savedContext.I savedContext.h \
     texture.I texture.N texture.h \
     textureContext.I textureContext.h \
     texturePool.I texturePool.h
    
  #define INCLUDED_SOURCES \
     LOD.cxx config_gobj.cxx drawable.cxx geom.cxx  \
     geomLine.cxx geomLinestrip.cxx geomPoint.cxx geomPolygon.cxx  \
     geomQuad.cxx geomSphere.cxx geomSprite.cxx geomTri.cxx  \
     geomTrifan.cxx geomTristrip.cxx imageBuffer.cxx material.cxx  \
     materialPool.cxx matrixLens.cxx orthographicLens.cxx  \
     perspectiveLens.cxx pixelBuffer.cxx lens.cxx  \
     savedContext.cxx texture.cxx textureContext.cxx texturePool.cxx

  #define INSTALL_HEADERS \
    LOD.I LOD.h config_gobj.h \
    drawable.h geom.I geom.h geomLine.h \
    geomLinestrip.h geomPoint.h geomPolygon.h geomQuad.h geomSphere.h \
    geomSprite.I geomSprite.h geomTri.h geomTrifan.h geomTristrip.h \
    geomprimitives.h imageBuffer.I imageBuffer.h material.I material.h \
    materialPool.I materialPool.h matrixLens.I matrixLens.h \
    orthographicLens.I orthographicLens.h perspectiveLens.I \
    perspectiveLens.h pixelBuffer.I pixelBuffer.h \
    lens.h lens.I \
    savedContext.I savedContext.h \
    texture.I texture.h \
    textureContext.I textureContext.h \
    texturePool.I texturePool.h

  #define IGATESCAN all

#end lib_target

#begin test_bin_target
  #define TARGET test_gobj
  #define LOCAL_LIBS \
    gobj putil

  #define SOURCES \
    test_gobj.cxx

#end test_bin_target

