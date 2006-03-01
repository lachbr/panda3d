#define OTHER_LIBS interrogatedb:c dconfig:c dtoolconfig:m \
                   dtoolutil:c dtoolbase:c dtool:m prc:c

#begin lib_target
  #define TARGET collide
  #define LOCAL_LIBS \
    tform gobj pgraph putil \
    pstatclient
    
  #define COMBINED_SOURCES $[TARGET]_composite1.cxx $[TARGET]_composite2.cxx    

  #define SOURCES \
    collisionEntry.I collisionEntry.h \
    collisionGeom.I collisionGeom.h \
    collisionHandler.h  \
    collisionHandlerEvent.I collisionHandlerEvent.h  \
    collisionHandlerFloor.I collisionHandlerFloor.h  \
    collisionHandlerGravity.I collisionHandlerGravity.h  \
    collisionHandlerPhysical.I collisionHandlerPhysical.h  \
    collisionHandlerPusher.I collisionHandlerPusher.h  \
    collisionHandlerQueue.h \
    collisionInvSphere.I collisionInvSphere.h \
    collisionLine.I collisionLine.h \
    collisionLevelState.I collisionLevelState.h \
    collisionNode.I collisionNode.h \
    collisionPlane.I collisionPlane.h  \
    collisionPolygon.I collisionPolygon.h \
    collisionRay.I collisionRay.h \
    collisionRecorder.I collisionRecorder.h \
    collisionSegment.I collisionSegment.h  \
    collisionSolid.I collisionSolid.h \
    collisionSphere.I collisionSphere.h \
    collisionTraverser.I collisionTraverser.h  \
    collisionTube.I collisionTube.h \
    collisionVisualizer.I collisionVisualizer.h \
    config_collide.h
    
 #define INCLUDED_SOURCES \
    collisionEntry.cxx \
    collisionGeom.cxx \
    collisionHandler.cxx \
    collisionHandlerEvent.cxx  \
    collisionHandlerFloor.cxx \
    collisionHandlerGravity.cxx \
    collisionHandlerPhysical.cxx  \
    collisionHandlerPusher.cxx \
    collisionHandlerQueue.cxx  \
    collisionLevelState.cxx \
    collisionInvSphere.cxx  \
    collisionLine.cxx \
    collisionNode.cxx \
    collisionPlane.cxx  \
    collisionPolygon.cxx \
    collisionRay.cxx \
    collisionRecorder.cxx \
    collisionSegment.cxx  \
    collisionSolid.cxx \
    collisionSphere.cxx  \
    collisionTraverser.cxx \
    collisionTube.cxx  \
    collisionVisualizer.cxx \
    config_collide.cxx 

  #define INSTALL_HEADERS \
    collisionEntry.I collisionEntry.h \
    collisionGeom.I collisionGeom.h \
    collisionHandler.h \
    collisionHandlerEvent.I collisionHandlerEvent.h \
    collisionHandlerFloor.I collisionHandlerFloor.h \
    collisionHandlerGravity.I collisionHandlerGravity.h \
    collisionHandlerPhysical.I collisionHandlerPhysical.h \
    collisionHandlerPusher.I collisionHandlerPusher.h \
    collisionHandlerQueue.h \
    collisionInvSphere.I collisionInvSphere.h \
    collisionLevelState.I collisionLevelState.h \
    collisionLine.I collisionLine.h \
    collisionNode.I collisionNode.h \
    collisionPlane.I collisionPlane.h \
    collisionPolygon.I collisionPolygon.h \
    collisionRay.I collisionRay.h \
    collisionRecorder.I collisionRecorder.h \
    collisionSegment.I collisionSegment.h \
    collisionSolid.I collisionSolid.h \
    collisionSphere.I collisionSphere.h \
    collisionTraverser.I collisionTraverser.h \
    collisionTube.I collisionTube.h \
    collisionVisualizer.I collisionVisualizer.h \
    config_collide.h


  #define IGATESCAN all

#end lib_target

#begin test_bin_target
  #define TARGET test_collide
  #define LOCAL_LIBS \
    collide
  #define OTHER_LIBS $[OTHER_LIBS] pystub

  #define SOURCES \
    test_collide.cxx

#end test_bin_target

