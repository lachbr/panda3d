from panda3d.core import NodePath, CardMaker, Vec4, Quat, Vec3, SamplerState, OmniBoundingVolume, BillboardEffect
from panda3d.core import CollisionBox, CollisionNode, CollisionTraverser, CollisionHandlerQueue, BitMask32, Point3
from panda3d.core import LPlane, LineSegs, AntialiasAttrib

from src.leveleditor import LEGlobals
from src.leveleditor import LEUtils
from src.leveleditor.viewport.ViewportType import VIEWPORT_3D_MASK
from src.leveleditor.math.Ray import Ray
from src.leveleditor.actions.EditObjectProperties import EditObjectProperties
from src.leveleditor.actions.ActionGroup import ActionGroup
from src.leveleditor.selection.SelectionType import SelectionModeTransform
from src.leveleditor.menu.KeyBind import KeyBind

from .BaseTransformTool import BaseTransformTool, Rollover, Ready, Down, Global, \
    Local, TransformWidget, TransformWidgetAxis
from .BoxTool import BoxAction, ResizeHandle

import math

from PyQt5 import QtWidgets, QtCore

class MoveWidgetAxis(TransformWidgetAxis):

    def __init__(self, widget, axis):
        TransformWidgetAxis.__init__(self, widget, axis)
        self.head = base.loader.loadModel("models/editor/arrow_head.bam")
        self.head.reparentTo(self)
        self.head.setY(0.6)
        self.head.setScale(0.7)

        baseSegs = LineSegs()
        baseSegs.setColor(1, 1, 1, 1)
        baseSegs.setThickness(2.0)
        baseSegs.moveTo(0, 0, 0)
        baseSegs.drawTo(0, 0.6, 0)
        self.base = self.attachNewNode(baseSegs.create())
        self.base.setAntialias(AntialiasAttrib.MLine)

    def cleanup(self):
        self.head.removeNode()
        self.head = None
        self.base.removeNode()
        self.base = None
        TransformWidgetAxis.cleanup(self)

    def getClickBox(self):
        return [Vec3(-0.06, 0.0, -0.06), Vec3(0.06, 0.8, 0.06)]

class MoveWidget(TransformWidget):

    def createAxis(self, axis):
        return MoveWidgetAxis(self, axis)

class MoveTool(BaseTransformTool):

    Name = "Move"
    ToolTip = "Move Tool"
    KeyBind = KeyBind.MoveTool
    Icon = "resources/icons/editor-move.png"

    def __init__(self, mgr):
        BaseTransformTool.__init__(self, mgr)
        self.transformType = SelectionModeTransform.Translate

    def createWidget(self):
        self.widget = MoveWidget(self)

    def filterHandle(self, handle):
        if base.selectionMgr.hasSelectedObjects():
            return False
        return True

    def getUpdatedProperties(self, obj, inst):
        return {"origin": inst.getPos(obj.np.getParent())}

    def getActionName(self):
        return "Move"

    def onMouseMoveTransforming3D(self, vp):
        # 3D is a little more complicated. We need to define a plane parallel to the selected
        # axis, intersect the line from our camera to the 3D mouse position against the plane,
        # and use the intersection point as the movement value.
        now = self.getPointOnGizmo()
        absolute = base.snapToGrid(self.preTransformStart + now - self.transformStart)
        self.setGizmoOrigin(absolute)
        self.moveBox(absolute - self.boxOriginOffset)
