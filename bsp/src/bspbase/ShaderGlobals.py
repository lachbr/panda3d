from panda3d.bsp import (VertexLitGenericSpec, LightmappedGenericSpec, UnlitGenericSpec,
                         UnlitNoMatSpec, CSMRenderSpec, SkyBoxSpec, DecalModulateSpec)

def getShaders():
    return [
        VertexLitGenericSpec(),
        LightmappedGenericSpec(),
        UnlitGenericSpec(),
        UnlitNoMatSpec(),
        CSMRenderSpec(),
        SkyBoxSpec(),
        DecalModulateSpec()
    ]
