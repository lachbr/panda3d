#define OTHER_LIBS interrogatedb:c dconfig:c dtoolconfig:m \
                   dtoolutil:c dtoolbase:c dtool:m

#begin lib_target
  #define TARGET particlesystem
  #define LOCAL_LIBS \
    physics sgraph sgattrib graph sgraphutil
    
  #define COMBINED_SOURCES $[TARGET]_composite1.cxx $[TARGET]_composite2.cxx    

  #define SOURCES \
     baseParticle.I baseParticle.h baseParticleEmitter.I  \
     baseParticleEmitter.h baseParticleFactory.I  \
     baseParticleFactory.h baseParticleRenderer.I  \
     baseParticleRenderer.h boxEmitter.I boxEmitter.h  \
     config_particlesystem.h discEmitter.I discEmitter.h  \
     geomParticleRenderer.I geomParticleRenderer.h lineEmitter.I  \
     lineEmitter.h lineParticleRenderer.I lineParticleRenderer.h  \
     orientedParticle.I orientedParticle.h  \
     orientedParticleFactory.I orientedParticleFactory.h  \
     particleSystem.I particleSystem.h particleSystemManager.I  \
     particleSystemManager.h pointEmitter.I pointEmitter.h  \
     pointParticle.h pointParticleFactory.h  \
     pointParticleRenderer.I pointParticleRenderer.h  \
     rectangleEmitter.I rectangleEmitter.h ringEmitter.I  \
     ringEmitter.h sparkleParticleRenderer.I  \
     sparkleParticleRenderer.h sphereSurfaceEmitter.I  \
     sphereSurfaceEmitter.h sphereVolumeEmitter.I  \
     sphereVolumeEmitter.h spriteParticleRenderer.I  \
     spriteParticleRenderer.h tangentRingEmitter.I  \
     tangentRingEmitter.h zSpinParticle.I zSpinParticle.h  \
     zSpinParticleFactory.I zSpinParticleFactory.h  \
     particleCommonFuncs.h  \
    
 #define INCLUDED_SOURCES \
     baseParticle.cxx baseParticleEmitter.cxx baseParticleFactory.cxx \
     baseParticleRenderer.cxx boxEmitter.cxx \
     config_particlesystem.cxx discEmitter.cxx \
     geomParticleRenderer.cxx lineEmitter.cxx \
     lineParticleRenderer.cxx orientedParticle.cxx \
     orientedParticleFactory.cxx particleSystem.cxx \
     particleSystemManager.cxx pointEmitter.cxx pointParticle.cxx \
     pointParticleFactory.cxx pointParticleRenderer.cxx \
     rectangleEmitter.cxx ringEmitter.cxx \
     sparkleParticleRenderer.cxx sphereSurfaceEmitter.cxx \
     sphereVolumeEmitter.cxx spriteParticleRenderer.cxx \
     tangentRingEmitter.cxx zSpinParticle.cxx \
     zSpinParticleFactory.cxx 

  #define INSTALL_HEADERS \
    baseParticle.I baseParticle.h baseParticleEmitter.I \
    baseParticleEmitter.h baseParticleFactory.I baseParticleFactory.h \
    baseParticleRenderer.I baseParticleRenderer.h boxEmitter.I \
    boxEmitter.h config_particlesystem.h discEmitter.I discEmitter.h \
    emitters.h geomParticleRenderer.I geomParticleRenderer.h \
    lineEmitter.I lineEmitter.h lineParticleRenderer.I \
    lineParticleRenderer.h orientedParticle.I orientedParticle.h \
    orientedParticleFactory.I orientedParticleFactory.h \
    particleSystem.I particleSystem.h particleSystemManager.I \
    particleSystemManager.h particlefactories.h particles.h \
    pointEmitter.I pointEmitter.h pointParticle.h \
    pointParticleFactory.h pointParticleRenderer.I \
    pointParticleRenderer.h rectangleEmitter.I rectangleEmitter.h \
    ringEmitter.I ringEmitter.h sparkleParticleRenderer.I \
    sparkleParticleRenderer.h sphereSurfaceEmitter.I \
    sphereSurfaceEmitter.h sphereVolumeEmitter.I sphereVolumeEmitter.h \
    spriteParticleRenderer.I spriteParticleRenderer.h \
    tangentRingEmitter.I tangentRingEmitter.h zSpinParticle.I \
    zSpinParticle.h zSpinParticleFactory.I zSpinParticleFactory.h \
    particleCommonFuncs.h

  #define IGATESCAN all

#end lib_target

