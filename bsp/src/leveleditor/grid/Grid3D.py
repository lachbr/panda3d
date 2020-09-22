from .Grid import Grid
from .GridSettings import GridSettings

from bsp.bspbase import BSPUtils

class Grid3D(Grid):

    def calcZoom(self):
        z = max(int(abs(self.viewport.camera.getZ() * 16)), 0.001)
        return BSPUtils.clamp(10000 / z, 0.001, 256)

    def shouldRender(self):
        return GridSettings.EnableGrid3D
