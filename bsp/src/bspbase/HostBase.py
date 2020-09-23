"""HostBase module: Contains the HostBase class, which is the main entry point
for all shows that utilize the BSP library."""

from direct.showbase.DirectObject import DirectObject
from direct.task import TaskManagerGlobal, Task
from direct.showbase import EventManagerGlobal
from direct.showbase import MessengerGlobal
from direct.showbase.Loader import Loader
from direct.directnotify.DirectNotifyGlobal import directNotify

from panda3d.core import ClockObject, ConfigVariableBool, PStatClient, AsyncTaskManager
from panda3d.core import TrueClock, loadPrcFile, loadPrcFileData, VirtualFileSystem

import builtins

# This is the default PRC data that we load for any app that uses HostBase.
# These values can be overridden/tweaked on a per-project basis by creating a
# etc/base.prc on the current working directory.
DefaultHostBasePRC = """
# GL stuff
load-display pandagl
gl-coordinate-system default
gl-force-fbo-color 0
gl-depth-zero-to-one 0
gl-force-depth-stencil 0
gl-compile-and-execute 1
gl-version 3 2
glsl-preprocess 1

allow-incomplete-render 1

framebuffer-srgb 1
framebuffer-alpha 0
framebuffer-float 0
framebuffer-multisample 0
multisamples 0
support-stencil 0 # Turn this on if we ever need stencil buffer
stencil-bits 0
depth-bits 24
shadow-depth-bits 32
color-bits 8 8 8
alpha-bits 0

default-model-extension .bam

allow-flatten-color 1

garbage-collect-states 1

hardware-animated-vertices 1
basic-shaders-only 0

pstats-eventmanager 1

flatten-collision-nodes 1

egg-load-old-curves 0

allow-async-bind 1
restore-initial-pose 0

preload-textures 1
preload-simple-textures 1
texture-compression 0
texture-minfilter mipmap
texture-magfilter linear

text-minfilter linear
text-magfilter linear
text-flatten 1
text-dynamic-merge 1

pssm-size 2048
pssm-shadow-depth-bias 0.0001
pssm-normal-offset-scale 3.0
pssm-softness-factor 2.0

interpolate-frames 1

support-ipv6 0

assert-abort 1

model-path .
"""

class HostBase(DirectObject):
    """ Main routine base class for client/server processes """

    notify = directNotify.newCategory("HostBase")

    def __init__(self):
        self.loadDefaultConfig()
        self.loadLocalConfig()

        if ConfigVariableBool("want-pstats", False):
            PStatClient.connect()

        # Set up some global objects
        self.globalClock = ClockObject.getGlobalClock()
        # We will manually manage the clock
        self.globalClock.setMode(ClockObject.MSlave)
        builtins.globalClock = self.globalClock

        self.vfs = VirtualFileSystem.getGlobalPtr()

        # For tasks that run every application frame
        self.taskMgr = TaskManagerGlobal.taskMgr
        builtins.taskMgr = self.taskMgr

        # For tasks that run every simulation tick
        self.simTaskMgr = Task.TaskManager()
        self.simTaskMgr.mgr = AsyncTaskManager("sim")
        builtins.simTaskmgr = self.simTaskMgr

        self.eventMgr = EventManagerGlobal.eventMgr
        builtins.eventMgr = self.eventMgr

        self.messenger = MessengerGlobal.messenger
        builtins.messenger = self.messenger

        self.loader = Loader(self)
        builtins.loader = self.loader

        builtins.base = self

        # What is the current frame number?
        self.frameCount = 0
        # Time at beginning of current frame
        self.frameTime = self.globalClock.getRealTime()
        # How long did the last frame take.
        self.deltaTime = 0

        #
        # Variables pertaining to simulation ticks.
        #

        self.prevRemainder = 0
        self.remainder = 0
        # What is the current overall simulation tick?
        self.tickCount = 0
        # How many ticks are we going to run this frame?
        self.totalTicksThisFrame = 0
        # How many ticks have we run so far this frame?
        self.currentTicksThisFrame = 0
        # What tick are we currently on this frame?
        self.currentFrameTick = 0
        # How many simulations ticks are we running per-second?
        self.ticksPerSec = 60
        self.intervalPerTick = 1.0 / self.ticksPerSec

    def loadDefaultPRCData(self, name, data):
        loadPrcFileData(name + "-default", data)

    def loadLocalPRCFile(self, name):
        # Loads a local config prc out of the etc folder on the current working
        # directory. Can be used to override default config values.
        return loadPrcFile("etc/" + name + ".prc")

    def loadDefaultConfig(self):
        self.loadDefaultPRCData("base", DefaultHostBasePRC)

    def loadLocalConfig(self):
        self.loadLocalPRCFile("base")

    def setTickRate(self, rate):
        self.ticksPerSec = rate
        self.intervalPerTick = 1.0 / self.ticksPerSec

    def ticksToTime(self, ticks):
        """ Returns the frame time of a tick number. """
        return self.intervalPerTick * float(ticks)

    def timeToTicks(self, time):
        """ Returns the tick number for a frame time. """
        return int(0.5 + float(time) / self.intervalPerTick)

    def isFinalTick(self):
        return self.currentTicksThisFrame == self.totalTicksThisFrame

    def shutdown(self):
        self.running = False

    def startup(self):
        self.eventMgr.restart()
        self.running = True

    def preRunFrame(self):
        pass

    def postRunFrame(self):
        pass

    def runFrame(self):
        #
        # First determine how many simulation ticks we should run.
        #

        self.prevRemainder = self.remainder
        if self.prevRemainder < 0.0:
            self.prevRemainder = 0.0

        self.remainder += self.deltaTime

        numTicks = 0
        if self.remainder >= self.intervalPerTick:
            numTicks = int(self.remainder / self.intervalPerTick)
            self.remainder -= numTicks * self.intervalPerTick

        self.totalTicksThisFrame = numTicks
        self.currentFrameTick = 0
        self.currentTicksThisFrame = 1

        #
        # Now run any simulation ticks.
        #

        for _ in range(numTicks):
            # Determine delta and frame time of this sim tick
            frameTime = self.intervalPerTick * self.tickCount
            deltaTime = self.intervalPerTick
            # Set it on the global clock for anything that uses it
            self.globalClock.setFrameTime(frameTime)
            self.globalClock.setDt(deltaTime)
            self.globalClock.setFrameCount(self.tickCount)

            # Step all simulation-bound tasks
            self.simTaskMgr.step()

            self.tickCount += 1
            self.currentFrameTick += 1
            self.currentTicksThisFrame += 1

        # Restore the true time for rendering and frame-bound stuff
        frameTime = self.tickCount * self.intervalPerTick + self.remainder
        self.globalClock.setFrameTime(frameTime)
        self.globalClock.setDt(self.deltaTime)
        self.globalClock.setFrameCount(self.frameCount)

        # And finally, step all frame-bound tasks
        self.taskMgr.step()

    def run(self):
        while self.running:

            # Manually advance the clock
            now = self.globalClock.getRealTime()
            self.deltaTime = now - self.frameTime
            self.frameTime = now

            self.globalClock.setFrameTime(self.frameTime)
            self.globalClock.setDt(self.deltaTime)
            self.globalClock.setFrameCount(self.frameCount)

            # Tick PStats if we are connected
            PStatClient.mainTick()

            self.preRunFrame()

            self.runFrame()

            self.postRunFrame()

            self.frameCount += 1
