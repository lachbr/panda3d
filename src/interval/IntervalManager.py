from pandac.PandaModules import *
from pandac import PandaModules
from direct.directnotify.DirectNotifyGlobal import *
from direct.showbase import EventManager
import Interval
import types
import fnmatch

class IntervalManager(CIntervalManager):

    # This is a Python-C++ hybrid class.  IntervalManager is a Python
    # extension of the C++ class CIntervalManager; the main purpose of
    # the Python extensions is to add support for Python-based
    # intervals (like MetaIntervals).

    if PandaModules.__dict__.has_key("Dtool_PyNavtiveInterface"):
        def __init__(self, globalPtr = 0):
            # Pass globalPtr == 1 to the constructor to trick it into
            # "constructing" a Python wrapper around the global
            # CIntervalManager object.
            ##self.cObj = CIntervalManager.getGlobalPtr()
            ##Dtool_BarrowThisRefrence(self, self.cObj)
            ##self.dd = self
            if globalPtr:
                self.cObj = CIntervalManager.getGlobalPtr()
                Dtool_BarrowThisRefrence(self, self.cObj)
                self.dd = self
            else:
                CIntervalManager.__init__(self)
            self.eventQueue = EventQueue()
            self.MyEventmanager = EventManager.EventManager(self.eventQueue)
            self.setEventQueue(self.eventQueue)
            self.ivals = []
            self.removedIvals = {}
    else:  ## the old interface
        def __init__(self, globalPtr = 0):
            # Pass globalPtr == 1 to the constructor to trick it into
            # "constructing" a Python wrapper around the global
            # CIntervalManager object.
            if globalPtr:
                #CIntervalManager.__init__(self, None)
                cObj = CIntervalManager.getGlobalPtr()
                self.this = cObj.this
                self.userManagesMemory = 0
            else:
                CIntervalManager.__init__(self)
            # Set up a custom event queue for handling C++ events from
            # intervals.
            self.eventQueue = EventQueue()
            self.MyEventmanager = EventManager.EventManager(self.eventQueue)
            self.setEventQueue(self.eventQueue)
            self.ivals = []
            self.removedIvals = {}



    def addInterval(self, interval):
        index = self.addCInterval(interval, 1)
        self.__storeInterval(interval, index)

    def removeInterval(self, interval):
        index = self.findCInterval(interval.getName())
        if index >= 0:
            self.removeCInterval(index)
            self.ivals[index] = None
            return 1
        return 0

    def getInterval(self, name):
        index = self.findCInterval(name)
        if index >= 0:
            return self.ivals[index]
        return None

    def finishIntervalsMatching(self, pattern):
        count = 0
        maxIndex = self.getMaxIndex()
        for index in range(maxIndex):
            ival = self.getCInterval(index)
            if ival and \
               fnmatch.fnmatchcase(ival.getName(), pattern):
                # Finish and remove this interval.  Finishing it
                # automatically removes it.
                count += 1
                if self.ivals[index]:
                    # Finish the python version if we have it
                    self.ivals[index].finish()
                else:
                    # Otherwise, it's a C-only interval.
                    ival.finish()

        return count


    def step(self):
        # This method should be called once per frame to perform all
        # of the per-frame processing on the active intervals.
        # Call C++ step, then do the Python stuff.
        CIntervalManager.step(self)
        self.__doPythonCallbacks()

    def interrupt(self):
        # This method should be called during an emergency cleanup
        # operation, to automatically pause or finish all active
        # intervals tagged with autoPause or autoFinish set true.
        # Call C++ interrupt, then do the Python stuff.
        CIntervalManager.interrupt(self)
        self.__doPythonCallbacks()

    def __doPythonCallbacks(self):
        # This method does all of the required Python post-processing
        # after performing some C++-level action.
        # It is important to call all of the python callbacks on the
        # just-removed intervals before we call any of the callbacks
        # on the still-running intervals.
        index = self.getNextRemoval()
        while index >= 0:
            # We have to clear the interval first before we call
            # privPostEvent() on it, because the interval might itself
            # try to add a new interval.
            ival = self.ivals[index]
            self.ivals[index] = None
            ival.privPostEvent()
            index = self.getNextRemoval()

        index = self.getNextEvent()
        while index >= 0:
            self.ivals[index].privPostEvent()
            index = self.getNextEvent()

        # Finally, throw all the events on the custom event queue.
        # These are the done events that may have been generated in
        # C++.  We use a custom event queue so we can service all of
        # these immediately, rather than waiting for the global event
        # queue to be serviced (which might not be till next frame).
        self.MyEventmanager.doEvents()


    def __storeInterval(self, interval, index):
        while index >= len(self.ivals):
            self.ivals.append(None)
        assert self.ivals[index] == None or self.ivals[index] == interval
        self.ivals[index] = interval


    #def __repr__(self):
    #    return self.__str__()

# The global IntervalManager object.
ivalMgr = IntervalManager(1)

