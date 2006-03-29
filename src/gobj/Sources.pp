#define OTHER_LIBS interrogatedb:c dconfig:c dtoolconfig:m \
                   dtoolutil:c dtoolbase:c dtool:m prc:c
#define OSX_SYS_LIBS mx
#define USE_PACKAGES zlib

#begin lib_target
  #define TARGET gobj
  #define LOCAL_LIBS \
    pstatclient event linmath mathutil pnmimage gsgbase putil

  #define COMBINED_SOURCES $[TARGET]_composite1.cxx $[TARGET]_composite2.cxx 

  #define SOURCES \
    bufferContext.I bufferContext.h \
    bufferContextChain.I bufferContextChain.h \
    bufferResidencyTracker.I bufferResidencyTracker.h \
    config_gobj.h \
    geom.h geom.I \
    geomContext.I geomContext.h \
    geomEnums.h \
    geomMunger.h geomMunger.I \
    geomPrimitive.h geomPrimitive.I \
    geomTriangles.h \
    geomTristrips.h \
    geomTrifans.h \
    geomLines.h \
    geomLinestrips.h \
    geomPoints.h \
    geomVertexArrayData.h geomVertexArrayData.I \
    geomVertexArrayFormat.h geomVertexArrayFormat.I \
    geomCacheEntry.h geomCacheEntry.I \
    geomCacheManager.h geomCacheManager.I \
    geomVertexAnimationSpec.h geomVertexAnimationSpec.I \
    geomVertexData.h geomVertexData.I \
    geomVertexColumn.h geomVertexColumn.I \
    geomVertexFormat.h geomVertexFormat.I \
    geomVertexReader.h geomVertexReader.I \
    geomVertexRewriter.h geomVertexRewriter.I \
    geomVertexWriter.h geomVertexWriter.I \
    indexBufferContext.I indexBufferContext.h \
    internalName.I internalName.h \
    material.I material.h materialPool.I materialPool.h  \
    matrixLens.I matrixLens.h \
    occlusionQueryContext.I occlusionQueryContext.h \
    orthographicLens.I orthographicLens.h perspectiveLens.I  \
    perspectiveLens.h \
    preparedGraphicsObjects.I preparedGraphicsObjects.h \
    lens.h lens.I \
    queryContext.I queryContext.h \
    savedContext.I savedContext.h \
    shaderContext.h shaderContext.I \
    shaderExpansion.h shaderExpansion.I \
    sliderTable.I sliderTable.h \
    texture.I texture.h \
    textureContext.I textureContext.h \
    texturePool.I texturePool.h \
    textureStage.I textureStage.h \
    transformBlend.I transformBlend.h \
    transformBlendTable.I transformBlendTable.h \
    transformTable.I transformTable.h \
    userVertexSlider.I userVertexSlider.h \
    userVertexTransform.I userVertexTransform.h \
    vertexBufferContext.I vertexBufferContext.h \
    vertexSlider.I vertexSlider.h \
    vertexTransform.I vertexTransform.h \
    videoTexture.I videoTexture.h
    
  #define INCLUDED_SOURCES \
    bufferContext.cxx \
    bufferContextChain.cxx \
    bufferResidencyTracker.cxx \
    config_gobj.cxx \
    geomContext.cxx \
    geom.cxx \
    geomEnums.cxx \
    geomMunger.cxx \
    geomPrimitive.cxx \
    geomTriangles.cxx \
    geomTristrips.cxx \
    geomTrifans.cxx \
    geomLines.cxx \
    geomLinestrips.cxx \
    geomPoints.cxx \
    geomVertexArrayData.cxx \
    geomVertexArrayFormat.cxx \
    geomCacheEntry.cxx \
    geomCacheManager.cxx \
    geomVertexAnimationSpec.cxx \
    geomVertexData.cxx \
    geomVertexColumn.cxx \
    geomVertexFormat.cxx \
    geomVertexReader.cxx \
    geomVertexRewriter.cxx \
    geomVertexWriter.cxx \
    indexBufferContext.cxx \
    material.cxx  \
    internalName.cxx \
    materialPool.cxx matrixLens.cxx \
    occlusionQuery.cxx \
    orthographicLens.cxx  \
    perspectiveLens.cxx \
    preparedGraphicsObjects.cxx \
    lens.cxx  \
    queryContext.cxx \
    savedContext.cxx \
    shaderContext.cxx \
    shaderExpansion.cxx \
    sliderTable.cxx \
    texture.cxx textureContext.cxx texturePool.cxx \
    textureStage.cxx \
    transformBlend.cxx \
    transformBlendTable.cxx \
    transformTable.cxx \
    userVertexSlider.cxx \
    userVertexTransform.cxx \
    vertexBufferContext.cxx \
    vertexSlider.cxx \
    vertexTransform.cxx \
    videoTexture.cxx

  #define INSTALL_HEADERS \
    bufferContext.I bufferContext.h \
    bufferContextChain.I bufferContextChain.h \
    bufferResidencyTracker.I bufferResidencyTracker.h \
    config_gobj.h \
    geom.I geom.h \
    textureContext.I textureContext.h \
    geom.h geom.I \
    geomContext.I geomContext.h \
    geomEnums.h \
    geomMunger.h geomMunger.I \
    geomPrimitive.h geomPrimitive.I \
    geomTriangles.h \
    geomTristrips.h \
    geomTrifans.h \
    geomLines.h \
    geomLinestrips.h \
    geomPoints.h \
    geomVertexArrayData.h geomVertexArrayData.I \
    geomVertexArrayFormat.h geomVertexArrayFormat.I \
    geomCacheEntry.h geomCacheEntry.I \
    geomCacheManager.h geomCacheManager.I \
    geomVertexAnimationSpec.h geomVertexAnimationSpec.I \
    geomVertexData.h geomVertexData.I \
    geomVertexColumn.h geomVertexColumn.I \
    geomVertexFormat.h geomVertexFormat.I \
    geomVertexReader.h geomVertexReader.I \
    geomVertexRewriter.h geomVertexRewriter.I \
    geomVertexWriter.h geomVertexWriter.I \
    indexBufferContext.I indexBufferContext.h \
    internalName.I internalName.h \
    material.I material.h \
    materialPool.I materialPool.h matrixLens.I matrixLens.h \
    occlusionQueryContext.I occlusionQueryContext.h \
    orthographicLens.I orthographicLens.h perspectiveLens.I \
    perspectiveLens.h \
    preparedGraphicsObjects.I preparedGraphicsObjects.h \
    lens.h lens.I \
    queryContext.I queryContext.h \
    savedContext.I savedContext.h \
    shaderContext.h shaderContext.I \
    shaderExpansion.h shaderExpansion.I \
    sliderTable.I sliderTable.h \
    texture.I texture.h \
    textureContext.I textureContext.h \
    texturePool.I texturePool.h \
    textureStage.I textureStage.h \
    transformBlend.I transformBlend.h \
    transformBlendTable.I transformBlendTable.h \
    transformTable.I transformTable.h \
    userVertexSlider.I userVertexSlider.h \
    userVertexTransform.I userVertexTransform.h \
    vertexBufferContext.I vertexBufferContext.h \
    vertexSlider.I vertexSlider.h \
    vertexTransform.I vertexTransform.h \
    videoTexture.I videoTexture.h


  #define IGATESCAN all

#end lib_target

#begin test_bin_target
  #define TARGET test_gobj
  #define LOCAL_LIBS \
    gobj putil

  #define SOURCES \
    test_gobj.cxx

#end test_bin_target

