from .Grid import Grid
from .GridSettings import GridSettings

from src.coginvasion.globals import CIGlobals

class Grid3D(Grid):

    def calcZoom(self):
        z = max(int(abs(self.viewport.camera.getZ() * 16)), 0.001)
        return CIGlobals.clamp(10000 / z, 0.001, 256)

    def shouldRender(self):
        return GridSettings.EnableGrid3D
