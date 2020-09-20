from panda3d.core import BitMask32, CullBinManager, Vec4

#
# Collision masks
#

# Objects
ObjectMask = BitMask32.bit(0)
# Solid faces
FaceMask = BitMask32.bit(1)

# View-specific masks
Mask3D = BitMask32.bit(2)
Mask2D = BitMask32.bit(3)

# Non-object masks
ManipulatorMask = BitMask32.bit(4)

SelectedSort = 1
BoxSort = 2
WidgetSort = 3

PreviewBrush2DColor = Vec4(51 / 255, 223 / 255, 1.0, 1.0)

AppName = "Foundry"
