from PandaObject import *
from DirectGeometry import *

class DirectGrid(NodePath,PandaObject):
    def __init__(self, direct):
        # Initialize superclass
        NodePath.__init__(self)
        self.assign(hidden.attachNewNode( NamedNode('DirectGrid')))

        # Record handle to direct session
        self.direct = direct

	# Load up grid parts to initialize grid object
	# Polygon used to mark grid plane
	self.gridBack = loader.loadModel('misc/gridBack')
	self.gridBack.reparentTo(self)
	self.gridBack.setColor(0.5,0.5,0.5,0.5)

	# Grid Lines
        self.lines = self.attachNewNode(NamedNode('gridLines'))
	self.minorLines = LineNodePath(self.lines)
        self.minorLines.lineNode.setName('minorLines')
	self.minorLines.setColor(VBase4(0.3,0.55,1,1))
	self.minorLines.setThickness(1)

	self.majorLines = LineNodePath(self.lines)
        self.majorLines.lineNode.setName('majorLines')
	self.majorLines.setColor(VBase4(0.3,0.55,1,1))
	self.majorLines.setThickness(5)

	self.centerLines = LineNodePath(self.lines)
        self.centerLines.lineNode.setName('centerLines')
	self.centerLines.setColor(VBase4(1,0,0,0))
	self.centerLines.setThickness(3)

	# Small marker to hilight snap-to-grid point
	self.snapMarker = loader.loadModel('misc/sphere')
	self.snapMarker.node().setName('gridSnapMarker')
	self.snapMarker.reparentTo(self)
	self.snapMarker.setColor(1,0,0,1)
	self.snapMarker.setScale(0.3)
        self.snapPos = Point3(0)

	# Initialize Grid characteristics
        self.fXyzSnap = 1
        self.fHprSnap = 1
	self.gridSize = 100.0
	self.gridSpacing = 5.0
        self.snapAngle = 15.0
        self.enable()

    def enable(self):
        self.reparentTo(render)
        self.accept('selectedNodePath', self.selectGridBackParent)
        self.updateGrid()

    def disable(self):
        self.reparentTo(hidden)
        self.ignore('selectedNodePath')

    def selectGridBackParent(self, nodePath):
        if nodePath.getNodePathName() == 'GridBack':
            self.direct.select(self)

    def updateGrid(self):
	# Update grid lines based upon current grid spacing and grid size
	# First reset existing grid lines
	self.minorLines.reset()
	self.majorLines.reset()
	self.centerLines.reset()

	# Now redraw lines
	numLines = math.ceil(self.gridSize/self.gridSpacing)
	scaledSize = numLines * self.gridSpacing
 
        center = self.centerLines
        minor = self.minorLines
        major = self.majorLines
        for i in range(-numLines,numLines + 1):
            if i == 0:
                center.moveTo(i * self.gridSpacing, -scaledSize, 0)
                center.drawTo(i * self.gridSpacing, scaledSize, 0)
                center.moveTo(-scaledSize, i * self.gridSpacing, 0)
                center.drawTo(scaledSize, i * self.gridSpacing, 0)
            else:
                if (i % 5) == 0:
                    major.moveTo(i * self.gridSpacing, -scaledSize, 0)
                    major.drawTo(i * self.gridSpacing, scaledSize, 0)
                    major.moveTo(-scaledSize, i * self.gridSpacing, 0)
                    major.drawTo(scaledSize, i * self.gridSpacing, 0)
                else:
                    minor.moveTo(i * self.gridSpacing, -scaledSize, 0)
                    minor.drawTo(i * self.gridSpacing, scaledSize, 0)
                    minor.moveTo(-scaledSize, i * self.gridSpacing, 0)
                    minor.drawTo(scaledSize, i * self.gridSpacing, 0)

        center.create()
        minor.create()
        major.create()
        self.gridBack.setScale(scaledSize)
        
    def setXyzSnap(self, fSnap):
        self.fXyzSnap = fSnap

    def getXyzSnap(self):
        return self.fXyzSnap

    def setHprSnap(self, fSnap):
        self.fHprSnap = fSnap

    def getHprSnap(self):
        return self.fHprSnap

    def computeSnapPoint(self, point):
        # Start of with current point
        self.snapPos.assign(point)
        # Snap if necessary
        if self.fXyzSnap:
            self.snapPos.set(
                roundTo(self.snapPos[0], self.gridSpacing),
                roundTo(self.snapPos[1], self.gridSpacing),
                roundTo(self.snapPos[2], self.gridSpacing))
            
	# Move snap marker to this point
	self.snapMarker.setPos(self.snapPos)
	
	# Return the hit point
	return self.snapPos

    def computeSnapAngle(self, angle):
        return roundTo(angle, self.snapAngle)

    def setSnapAngle(self, angle):
        self.snapAngle = angle

    def getSnapAngle(self):
        return self.snapAngle

    def setGridSpacing(self, spacing):
        self.gridSpacing = spacing
        self.updateGrid()
        
    def getGridSpacing(self):
        return self.gridSpacing

    def setSize(self, size):
	# Set size of grid back and redraw lines
        self.gridSize = size
	self.updateGrid()
