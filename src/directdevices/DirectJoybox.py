""" Class used to create and control joybox device """
from PandaObject import *
from DirectDeviceManager import *
from DirectUtil import *
import OnscreenText

"""
TODO:
Handle interaction between widget, followSelectedTask and updateTask
"""

# BUTTONS
L_STICK = 0
L_UPPER = 1
L_LOWER = 2
R_STICK = 3
R_UPPER = 4
R_LOWER = 5
# ANALOGS
NULL_AXIS = -1
L_LEFT_RIGHT = 0
L_FWD_BACK = 1
L_TWIST = 2
L_SLIDE = 3
R_LEFT_RIGHT = 4
R_FWD_BACK = 5
R_TWIST = 6
R_SLIDE = 7

JOYBOX_MIN = ANALOG_MIN + ANALOG_DEADBAND
JOYBOX_MAX = ANALOG_MAX - ANALOG_DEADBAND
JOYBOX_RANGE = JOYBOX_MAX - JOYBOX_MIN

class DirectJoybox(PandaObject):
    joyboxCount = 0
    xyzMultiplier = 1.0
    hprMultiplier = 1.0
    def __init__(self, device = 'CerealBox', nodePath = direct.camera):
        # See if device manager has been initialized
        if direct.deviceManager == None:
            direct.deviceManager = DirectDeviceManager()
        # Set name
        DirectJoybox.joyboxCount += 1
        self.name = 'Joybox-' + `DirectJoybox.joyboxCount`
        # Get buttons and analogs
        self.device = device
        self.analogs = direct.deviceManager.createAnalogs(self.device)
        self.buttons = direct.deviceManager.createButtons(self.device)
        self.aList = [0,0,0,0,0,0,0,0]
        self.bList = [0,0,0,0,0,0,0,0,0,0,0,0]
        # For joybox fly mode
        # Default is joe mode
        self.mapping = [R_LEFT_RIGHT, R_FWD_BACK, L_FWD_BACK,
                        R_TWIST, L_TWIST, NULL_AXIS]
        self.modifier = [1,1,1,-1,-1,0]
        # Initialize time
        self.lastTime = globalClock.getFrameTime()
        # Record node path
        self.nodePath = nodePath
        # Ref CS for orbit mode
        self.refCS = direct.cameraControl.coaMarker
        self.tempCS = direct.group.attachNewNode('JoyboxTempCS')
        # Text object to display current mode
        self.readout = OnscreenText.OnscreenText(
            pos = (-0.9, 0.95),
            font = direct.font,
            mayChange = 1)
        # List of functions to cycle through
        self.modeList = [self.joeMode, self.driveMode, self.orbitMode]
        # Pick initial mode
        self.updateFunc = self.joyboxFly
        self.modeName = 'Joe Mode'
        # Auxiliary data
        self.auxData = []
        # Button registry
        self.addButtonEvents()
        # Spawn update task
        self.enable()
    
    def enable(self):
        # Kill existing task
        self.disable()
        # Accept button events
        self.acceptSwitchModeEvent()
        self.acceptUprightCameraEvent()
        # Update task
        taskMgr.add(self.updateTask, self.name + '-updateTask')
    
    def disable(self):
        taskMgr.remove(self.name + '-updateTask')
        # Ignore button events
        self.ignoreSwitchModeEvent()
        self.ignoreUprightCameraEvent()

    def destroy(self):
        self.disable()
        self.tempCS.removeNode()

    def addButtonEvents(self):
        self.breg = ButtonRegistry.ptr()
        # MRM: Hard coded!
        for i in range(8):
            self.buttons.setButtonMap(
                i, self.breg.getButton(self.getEventName(i)))
        self.eventThrower = self.buttons.getNodePath().attachNewNode(
            ButtonThrower('JB Button Thrower'))
    
    def setNodePath(self, nodePath):
        self.nodePath = nodePath

    def getNodePath(self):
        return self.nodePath
    def setRefCS(self, refCS):
        self.refCS = refCS
    def getRefCS(self):
        return self.refCS
    def getEventName(self, index):
        return self.name + '-button-' + `index`
    def setXyzMultiplier(self, multiplier):
        DirectJoybox.xyzMultiplier = multiplier
    def getXyzMultiplier(self):
        return DirectJoybox.xyzMultiplier
    def setHprMultiplier(self, multiplier):
        DirectJoybox.hprMultiplier = multiplier
    def getHprMultiplier(self):
        return DirectJoybox.hprMultiplier

    def updateTask(self, state):
        # old optimization
        #self.updateValsUnrolled()
        self.updateVals()
        self.updateFunc()
        return Task.cont
    
    def updateVals(self):
        # Update delta time
        cTime = globalClock.getFrameTime()
        self.deltaTime = cTime - self.lastTime
        self.lastTime = cTime
        # Update analogs
        for i in range(len(self.analogs)):
            self.aList[i] = self.normalizeChannel(i)
        # Update buttons
        for i in range(len(self.buttons)):
            try:
                self.bList[i] = self.buttons[i]
            except IndexError:
                # That channel may not have been updated yet
                self.bList[i] = 0
    
    def updateValsUnrolled(self):
        # Update delta time
        cTime = globalClock.getFrameTime()
        self.deltaTime = cTime - self.lastTime
        self.lastTime = cTime
        # Update analogs
        for chan in range(len(self.analogs)):
            val = self.analogs.getControlState(chan)
            # Zero out values in deadband
            if (val < 0):
                val = min(val + ANALOG_DEADBAND, 0.0)
            else:
                val = max(val - ANALOG_DEADBAND, 0.0)
            # Scale up rotating knob values
            if (chan == L_TWIST) or (chan == R_TWIST):
                val *= 3.0
            # Now clamp value between minVal and maxVal
            val = CLAMP(val, JOYBOX_MIN, JOYBOX_MAX)
            self.aList[chan] = 2.0*((val - JOYBOX_MIN)/JOYBOX_RANGE) - 1
        # Update buttons
        for i in range(len(self.buttons)):
            try:
                self.bList[i] = self.buttons.getButtonState(i)
            except IndexError:
                # That channel may not have been updated yet
                self.bList[i] = 0
    
    def acceptSwitchModeEvent(self, button = R_UPPER):
        self.accept(self.getEventName(button), self.switchMode)
    def ignoreSwitchModeEvent(self, button = R_UPPER):
        self.ignore(self.getEventName(button))
        
    def switchMode(self):
        try:
            # Get current mode
            self.modeFunc = self.modeList[0]
            # Rotate mode list
            self.modeList = self.modeList[1:] + self.modeList[:1]
            # Call new mode
            self.modeFunc()
        except IndexError:
            pass

    def showMode(self, modeText):
        def hideText(state, s = self):
            s.readout.setText('')
            return Task.done
        taskMgr.remove(self.name + '-showMode')
        # Update display
        self.readout.setText(modeText)
        t = taskMgr.doMethodLater(3.0, hideText, self.name + '-showMode')
        t.uponDeath = hideText

    def acceptUprightCameraEvent(self, button = L_UPPER):
        self.accept(self.getEventName(button),
                    direct.cameraControl.orbitUprightCam)
    def ignoreUprightCameraEvent(self, button = L_UPPER):
        self.ignore(self.getEventName(button))

    def setMode(self, func, name):
        self.disable()
        self.updateFunc = func
        self.modeName = name
        self.showMode(self.modeName)
        self.enable()
        
    def joyboxFly(self):
        # Do nothing if no nodePath selected
        if self.nodePath == None:
            return
        hprScale = ((self.aList[L_SLIDE] + 1.0) *
                    50.0 * DirectJoybox.hprMultiplier)
        posScale = ((self.aList[R_SLIDE] + 1.0) *
                    50.0 * DirectJoybox.xyzMultiplier)
        def getAxisVal(index, s = self):
            try:
                return s.aList[s.mapping[index]]
            except IndexError:
                # If it is a null axis return 0
                return 0.0
        x = getAxisVal(0) * self.modifier[0]
        y = getAxisVal(1) * self.modifier[1]
        z = getAxisVal(2) * self.modifier[2]
        pos = Vec3(x,y,z) * (posScale * self.deltaTime)

        h = getAxisVal(3) * self.modifier[3]
        p = getAxisVal(4) * self.modifier[4]
        r = getAxisVal(5) * self.modifier[5]
        hpr = Vec3(h,p,r) * (hprScale * self.deltaTime)
        # Move node path
        self.nodePath.setPosHpr(self.nodePath, pos, hpr)

    def joeMode(self):
        self.mapping = [R_LEFT_RIGHT, R_FWD_BACK, L_FWD_BACK,
                        R_TWIST, L_TWIST, NULL_AXIS]
        self.modifier = [1,1,1,-1,-1,0]
        self.setMode(self.joyboxFly, 'Joe Mode')

    def lucMode(self):
        self.mapping = [R_LEFT_RIGHT, R_FWD_BACK, L_FWD_BACK,
                        R_TWIST, L_TWIST, L_LEFT_RIGHT]
        self.modifier = [1,1,1,-1,-1,0]
        self.setMode(self.joyboxFly, 'Luc Mode')

    def driveMode(self):
        self.mapping = [L_LEFT_RIGHT, R_FWD_BACK, R_TWIST,
                        R_LEFT_RIGHT, L_FWD_BACK, NULL_AXIS]
        self.modifier = [1,1,-1,-1,-1,0]
        self.setMode(self.joyboxFly, 'Drive Mode')

    def lookAtMode(self):
        self.mapping = [R_LEFT_RIGHT, R_TWIST, R_FWD_BACK,
                        L_LEFT_RIGHT, L_FWD_BACK, NULL_AXIS]
        self.modifier = [1,1,1,-1,1,0]
        self.setMode(self.joyboxFly, 'Look At Mode')

    def lookAroundMode(self):
        self.mapping = [NULL_AXIS, NULL_AXIS, NULL_AXIS,
                        R_LEFT_RIGHT, R_FWD_BACK, NULL_AXIS]
        self.modifier = [0,0,0,-1,-1,0]
        self.setMode(self.joyboxFly, 'Lookaround Mode')

    def demoMode(self):
        self.mapping = [R_LEFT_RIGHT, R_FWD_BACK, L_FWD_BACK,
                        R_TWIST, NULL_AXIS, NULL_AXIS]
        self.modifier = [1,1,1,-1,0,0]
        self.setMode(self.joyboxFly, 'Demo Mode')

    def hprXyzMode(self):
        self.mapping = [R_LEFT_RIGHT, R_FWD_BACK, R_TWIST,
                        L_TWIST, L_FWD_BACK, L_LEFT_RIGHT]
        self.modifier = [1,1,-1,-1,-1,1]
        self.setMode(self.joyboxFly, 'HprXyz Mode')

    def walkthruMode(self):
        self.mapping = [R_LEFT_RIGHT, R_FWD_BACK, L_TWIST,
                        R_TWIST, L_FWD_BACK, L_LEFT_RIGHT]
        self.modifier = [1,1,-1,-1,-1, 1]
        self.setMode(self.joyboxFly, 'Walkthru Mode')

    def spaceMode(self):
        self.setMode(self.spaceFly, 'Space Mode')

    def spaceFly(self):
        # Do nothing if no nodePath selected
        if self.nodePath == None:
            return
        hprScale = (self.normalizeChannel(L_SLIDE, 0.1, 100) *
                    DirectJoybox.hprMultiplier)
        posScale = (self.normalizeChannel(R_SLIDE, 0.1, 100) *
                    DirectJoybox.xyzMultiplier)
        dr = -1 * hprScale * self.aList[R_TWIST] * self.deltaTime
        dp = -1 * hprScale * self.aList[R_FWD_BACK] * self.deltaTime
        dh = -1 * hprScale * self.aList[R_LEFT_RIGHT] * self.deltaTime
        self.nodePath.setHpr(self.nodePath, dh, dp, dr)
        dy = posScale * self.aList[L_FWD_BACK] * self.deltaTime
        self.nodePath.setY(self.nodePath, dy)

    def planetMode(self, auxData = []):
        self.auxData = auxData
        self.setMode(self.planetFly, 'Space Mode')

    def planetFly(self):
        # Do nothing if no nodePath selected
        if self.nodePath == None:
            return
        hprScale = (self.normalizeChannel(L_SLIDE, 0.1, 100) *
                    DirectJoybox.hprMultiplier)
        posScale = (self.normalizeChannel(R_SLIDE, 0.1, 100) *
                    DirectJoybox.xyzMultiplier)
        dr = -1 * hprScale * self.aList[R_TWIST] * self.deltaTime
        dp = -1 * hprScale * self.aList[R_FWD_BACK] * self.deltaTime
        dh = -1 * hprScale * self.aList[R_LEFT_RIGHT] * self.deltaTime
        self.nodePath.setHpr(self.nodePath, dh, dp, dr)
        dy = posScale * self.aList[L_FWD_BACK] * self.deltaTime
        dPos = VBase3(0,dy,0)
        for planet, radius in self.auxData:
            # Are we within min radius?
            # How far above planet are we?
            np2planet = Vec3(self.nodePath.getPos(planet))
            # Compute dist
            offsetDist = np2planet.length()
            # Above threshold, leave velocity vec as is
            if offsetDist > (1.2 * radius):
                pass
            else:
                # Getting close, slow things down
                # Compute normal vector through node Path
                oNorm = Vec3()
                oNorm.assign(np2planet)
                oNorm.normalize()
                # Xform fly vec to planet space
                dPlanet = self.nodePath.getMat(planet).xformVec(Vec3(0, dy, 0))
                # Compute radial component of fly vec
                dotProd = oNorm.dot(dPlanet)
                if dotProd < 0:
                    # Trying to fly below radius, compute radial component
                    radialComponent = oNorm * dotProd
                    # How far above?
                    above = offsetDist - radius
                    # Set sf accordingly
                    sf = max(1.0 - (max(above, 0.0)/(0.2 * radius)), 0.0)
                    # Subtract scaled radial component
                    dPlanet -= radialComponent * (sf * sf)
                    #dPlanet -= radialComponent
                    # Convert back to node path space
                    dPos.assign(planet.getMat(self.nodePath).xformVec(dPlanet))
        # Set pos accordingly
        self.nodePath.setPos(self.nodePath, dPos)

    def orbitMode(self):
        self.setMode(self.orbitFly, 'Orbit Mode')

    def orbitFly(self):
        # Do nothing if no nodePath selected
        if self.nodePath == None:
            return
        hprScale = (self.normalizeChannel(L_SLIDE, 0.1, 100) *
                    DirectJoybox.hprMultiplier)
        posScale = (self.normalizeChannel(R_SLIDE, 0.1, 100) *
                    DirectJoybox.xyzMultiplier)
        r = -0.01 * posScale * self.aList[R_TWIST] * self.deltaTime
        rx = hprScale * self.aList[R_LEFT_RIGHT] * self.deltaTime
        ry = -hprScale * self.aList[R_FWD_BACK] * self.deltaTime
        x = posScale * self.aList[L_LEFT_RIGHT] * self.deltaTime
        z = posScale * self.aList[L_FWD_BACK] * self.deltaTime
        h = -1 * hprScale * self.aList[L_TWIST] * self.deltaTime
        # Move dcs
        self.nodePath.setX(self.nodePath, x)
        self.nodePath.setZ(self.nodePath, z)
        self.nodePath.setH(self.nodePath, h)
        self.orbitNode(rx, ry, 0)
        pos = self.nodePath.getPos(self.refCS)
        if Vec3(pos).length() < 0.005:
            pos.set(0,-0.01, 0)
        # Now move on out
        pos.assign(pos * (1 + r))
        self.nodePath.setPos(self.refCS, pos)

    def orbitNode(self, h, p, r):
        # Position the temp node path at the ref CS
        self.tempCS.setPos(self.refCS, 0, 0, 0)
        # Orient the temp node path to align with the orbiting node path
        self.tempCS.setHpr(self.nodePath, 0, 0, 0)
        # Record the position of the orbiter wrt the helper
        pos = self.nodePath.getPos(self.tempCS)
        # Turn the temp node path
        self.tempCS.setHpr(self.tempCS, h, p, r)
        # Position the orbiter "back" to its position wrt the helper
        self.nodePath.setPos(self.tempCS, pos)
        # Restore the original hpr of the orbiter
        self.nodePath.setHpr(self.tempCS, 0, 0, 0)


    # We need to override the DirectAnalog normalizeChannel to
    # correct the ranges of the two twist axes of the joybox.
    def normalizeChannel(self, chan, minVal = -1, maxVal = 1):
        try:
            if (chan == L_TWIST) or (chan == R_TWIST):
                # These channels have reduced range
                return self.analogs.normalize(
                    self.analogs.getControlState(chan), minVal, maxVal, 3.0)
            else:
                return self.analogs.normalize(
                    self.analogs.getControlState(chan), minVal, maxVal)
        except IndexError:
            return 0.0

            
