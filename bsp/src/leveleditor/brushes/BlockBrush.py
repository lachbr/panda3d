from .BaseBrush import BaseBrush

from src.leveleditor import LEUtils

class BlockBrush(BaseBrush):
    Name = "Block"

    def create(self, generator, mins, maxs, material, roundDecimals, temp = False):
        faces = LEUtils.getBoxFaces(mins, maxs, roundDecimals)
        return [self.makeSolid(generator, faces, material, temp)]
