"""LerpInterval module: contains the LerpInterval class"""

from PandaModules import *
from DirectNotifyGlobal import *
import Interval
import LerpBlendHelpers

#
# Most of the intervals defined in this module--the group up here at
# the front of the file--are now derived from a CInterval instead of
# an Interval, so they can run in the low-level C++ code at high
# performance.
#

class LerpNodePathInterval(CLerpNodePathInterval):
    # This is the base class for all of the lerps, defined below, that
    # affect a property on a NodePath, like pos or hpr.
    lerpNodePathNum = 1

    def __init__(self, name, duration, blendType, node, other):
        if name == None:
            name = '%s-%d' % (self.__class__.__name__, self.lerpNodePathNum)
            LerpNodePathInterval.lerpNodePathNum += 1

        blendType = self.stringBlendType(blendType)
        assert(blendType != self.BTInvalid)

        if other == None:
            other = NodePath()

        CLerpNodePathInterval.__init__(self, name, duration, blendType,
                                       node, other)

    def anyCallable(self, *params):
        # Returns true if any of the parameters listed is a callable
        # functor, false if none of them are.  This is used by derived
        # classes to determine if a functor was passed in for a
        # parameter.
        
        for param in params:
            if callable(param):
                return 1
        return 0

    def setupParam(self, func, param):
        # Stores the indicated parameter by passing it to the given
        # function (probably a C++ setter function).  If the param is
        # a callable functor, calls it; otherwise, uses the param
        # directly.
        if param != None:
            if callable(param):
                func(param())
            else:
                func(param)

#####################################################################
##
##  It is legal to pass in a functor for the any of the pos, hpr,
##  or scale parameters in the intervals below.  These will be
##  evaluated at the time the interval starts in order to determine
##  the actual final position.  However, doing so forces the entire
##  LerpInterval to be executed up in Python, instead of down in the
##  low-level C++ code, at a significant performance cost.
##
#####################################################################

class LerpPosInterval(LerpNodePathInterval):
    def __init__(self, node, duration, pos, startPos = None,
                 other = None, blendType = 'noBlend', name = None):
        LerpNodePathInterval.__init__(self, name, duration, blendType,
                                      node, other)

        # Check for functors in the input parameters.
        self.paramSetup = self.anyCallable(pos, startPos)
        if self.paramSetup:
            self.endPos = pos
            self.startPos = startPos
            self.inPython = 1
        else:
            self.setEndPos(pos)
            if startPos != None:
                self.setStartPos(startPos)

    def setT(self, t, event = Interval.IVAL_NONE):
        # This function is only used if Python functors were passed in
        # for some of the input parameters.
        if self.paramSetup and event == Interval.IVAL_INIT:
            self.setupParam(self.setEndPos, self.endPos)
            self.setupParam(self.setStartPos, self.startPos)
        LerpNodePathInterval.setT(self, t, event)
                

class LerpHprInterval(LerpNodePathInterval):
    def __init__(self, node, duration, hpr, startHpr = None,
                 other = None, blendType = 'noBlend', name = None):
        LerpNodePathInterval.__init__(self, name, duration, blendType,
                                      node, other)

        # Check for functors in the input parameters.
        self.paramSetup = self.anyCallable(hpr, startHpr)
        if self.paramSetup:
            self.endHpr = hpr
            self.startHpr = startHpr
            self.inPython = 1
        else:
            self.setEndHpr(hpr)
            if startHpr != None:
                self.setStartHpr(startHpr)

    def setT(self, t, event = Interval.IVAL_NONE):
        # This function is only used if Python functors were passed in
        # for some of the input parameters.
        if self.paramSetup and event == Interval.IVAL_INIT:
            self.setupParam(self.setEndHpr, self.endHpr)
            self.setupParam(self.setStartHpr, self.startHpr)
        LerpNodePathInterval.setT(self, t, event)

class LerpScaleInterval(LerpNodePathInterval):
    def __init__(self, node, duration, scale, startScale = None,
                 other = None, blendType = 'noBlend', name = None):
        LerpNodePathInterval.__init__(self, name, duration, blendType,
                                      node, other)
        # Check for functors in the input parameters.
        self.paramSetup = self.anyCallable(scale, startScale)
        if self.paramSetup:
            self.endScale = scale
            self.startScale = startScale
            self.inPython = 1
        else:
            self.setEndScale(scale)
            if startScale != None:
                self.setStartScale(startScale)

    def setT(self, t, event = Interval.IVAL_NONE):
        # This function is only used if Python functors were passed in
        # for some of the input parameters.
        if self.paramSetup and event == Interval.IVAL_INIT:
            self.setupParam(self.setEndScale, self.endScale)
            self.setupParam(self.setStartScale, self.startScale)
        LerpNodePathInterval.setT(self, t, event)

class LerpPosHprInterval(LerpNodePathInterval):
    def __init__(self, node, duration, pos, hpr,
                 startPos = None, startHpr = None,
                 other = None, blendType = 'noBlend', name = None):
        LerpNodePathInterval.__init__(self, name, duration, blendType,
                                      node, other)
        # Check for functors in the input parameters.
        self.paramSetup = self.anyCallable(pos, startPos, hpr, startHpr)
        if self.paramSetup:
            self.endPos = pos
            self.startPos = startPos
            self.endHpr = hpr
            self.startHpr = startHpr
            self.inPython = 1
        else:
            self.setEndPos(pos)
            if startPos != None:
                self.setStartPos(startPos)
            self.setEndHpr(hpr)
            if startHpr != None:
                self.setStartHpr(startHpr)

    def setT(self, t, event = Interval.IVAL_NONE):
        # This function is only used if Python functors were passed in
        # for some of the input parameters.
        if self.paramSetup and event == Interval.IVAL_INIT:
            self.setupParam(self.setEndPos, self.endPos)
            self.setupParam(self.setStartPos, self.startPos)
            self.setupParam(self.setEndHpr, self.endHpr)
            self.setupParam(self.setStartHpr, self.startHpr)
        LerpNodePathInterval.setT(self, t, event)

class LerpHprScaleInterval(LerpNodePathInterval):
    def __init__(self, node, duration, hpr, scale,
                 startHpr = None, startScale = None,
                 other = None, blendType = 'noBlend', name = None):
        LerpNodePathInterval.__init__(self, name, duration, blendType,
                                      node, other)

        # Check for functors in the input parameters.
        self.paramSetup = self.anyCallable(hpr, startHpr, scale, startScale)
        if self.paramSetup:
            self.endHpr = hpr
            self.startHpr = startHpr
            self.endScale = scale
            self.startScale = startScale
            self.inPython = 1
        else:
            self.setEndHpr(hpr)
            if startHpr != None:
                self.setStartHpr(startHpr)
            self.setEndScale(scale)
            if startScale != None:
                self.setStartScale(startScale)

    def setT(self, t, event = Interval.IVAL_NONE):
        # This function is only used if Python functors were passed in
        # for some of the input parameters.
        if self.paramSetup and event == Interval.IVAL_INIT:
            self.setupParam(self.setEndHpr, self.endHpr)
            self.setupParam(self.setStartHpr, self.startHpr)
            self.setupParam(self.setEndScale, self.endScale)
            self.setupParam(self.setStartScale, self.startScale)
        LerpNodePathInterval.setT(self, t, event)

class LerpPosHprScaleInterval(LerpNodePathInterval):
    def __init__(self, node, duration, pos, hpr, scale,
                 startPos = None, startHpr = None, startScale = None,
                 other = None, blendType = 'noBlend', name = None):
        LerpNodePathInterval.__init__(self, name, duration, blendType,
                                      node, other)
        # Check for functors in the input parameters.
        self.paramSetup = self.anyCallable(pos, startPos, hpr, startHpr, scale, startScale)
        if self.paramSetup:
            self.endPos = pos
            self.startPos = startPos
            self.endHpr = hpr
            self.startHpr = startHpr
            self.endScale = scale
            self.startScale = startScale
            self.inPython = 1
        else:
            self.setEndPos(pos)
            if startPos != None:
                self.setStartPos(startPos)
            self.setEndHpr(hpr)
            if startHpr != None:
                self.setStartHpr(startHpr)
            self.setEndScale(scale)
            if startScale != None:
                self.setStartScale(startScale)

    def setT(self, t, event = Interval.IVAL_NONE):
        # This function is only used if Python functors were passed in
        # for some of the input parameters.
        if self.paramSetup and event == Interval.IVAL_INIT:
            self.setupParam(self.setEndPos, self.endPos)
            self.setupParam(self.setStartPos, self.startPos)
            self.setupParam(self.setEndHpr, self.endHpr)
            self.setupParam(self.setStartHpr, self.startHpr)
            self.setupParam(self.setEndScale, self.endScale)
            self.setupParam(self.setStartScale, self.startScale)
        LerpNodePathInterval.setT(self, t, event)

class LerpColorScaleInterval(LerpNodePathInterval):
    def __init__(self, node, duration, colorScale, startColorScale = None,
                 other = None, blendType = 'noBlend', name = None):
        LerpNodePathInterval.__init__(self, name, duration, blendType,
                                      node, other)
        self.setEndColorScale(colorScale)
        if startColorScale != None:
            self.setStartColorScale(startColorScale)

class LerpColorInterval(LerpNodePathInterval):
    def __init__(self, node, duration, color, startColor = None,
                 other = None, blendType = 'noBlend', name = None):
        LerpNodePathInterval.__init__(self, name, duration, blendType,
                                      node, other)
        self.setEndColor(color)
        if startColor != None:
            self.setStartColor(startColor)


#
# The remaining intervals defined in this module are the old-school
# Python-based intervals.
#

class LerpInterval(Interval.Interval):
    # create LerpInterval DirectNotify category
    notify = directNotify.newCategory('LerpInterval')
    # Class methods
    def __init__(self, name, duration, functorFunc, blendType='noBlend'):
        """__init__(name, duration, functorFunc, blendType)
        """
        # Record instance variables
        self.lerp = None
        self.functorFunc = functorFunc
        self.blendType = self.getBlend(blendType)
        # Initialize superclass
        Interval.Interval.__init__(self, name, duration)

    def updateFunc(self, t, event = Interval.IVAL_NONE):
        """ updateFunc(t, event)
        """
        # Check to see if we need to create the lerp
        if (event == Interval.IVAL_INIT):
            self.lerp = Lerp(self.functorFunc(), self.duration, 
                                  self.blendType)
        # Make sure lerp exists
        if (not self.lerp):
            self.lerp = Lerp(self.functorFunc(), self.duration, 
                                  self.blendType)
        # Evaluate the lerp
        self.lerp.setT(t)
        # Print debug information
        self.notify.debug('updateFunc() - %s: t = %f' % (self.name, t))

    def getBlend(self, blendType):
        """__getBlend(self, string)
        Return the C++ blend class corresponding to blendType string
        """
        if (blendType == "easeIn"):
            return LerpBlendHelpers.easeIn
        elif (blendType == "easeOut"):
            return LerpBlendHelpers.easeOut
        elif (blendType == "easeInOut"):
            return LerpBlendHelpers.easeInOut
        elif (blendType == "noBlend"):
            return LerpBlendHelpers.noBlend
        else:
            raise Exception(
                'Error: LerpInterval.__getBlend: Unknown blend type')


class LerpFunctionInterval(Interval.Interval):
    """
    Class used to execute a function over time.  Function can access fromData
    and toData to perform blend.  If fromData and toData not specified, will
    execute the given function passing in values ranging from 0 to 1
    """

    # Interval counter
    lerpFunctionIntervalNum = 1
    # create LerpFunctionInterval DirectNotify category
    notify = directNotify.newCategory('LerpFunctionInterval')
    # Class methods
    def __init__(self, function, fromData = 0, toData = 1, duration = 0.0,
                 blendType = 'noBlend', extraArgs = [], name = None):
        """__init__(function, duration, fromData, toData, name)
        """
        # Record instance variables
        self.function = function
        self.fromData = fromData
        self.toData = toData
        self.blendType = self.getBlend(blendType)
        self.extraArgs = extraArgs
        # Generate unique name if necessary
        if (name == None):
            name = ('LerpFunctionInterval-%d' %
                    LerpFunctionInterval.lerpFunctionIntervalNum)
            LerpFunctionInterval.lerpFunctionIntervalNum += 1
        # Initialize superclass
        Interval.Interval.__init__(self, name, duration)

    def updateFunc(self, t, event = Interval.IVAL_NONE):
        """ updateFunc(t, event)
        """
        # Evaluate the function
        if (t >= self.duration):
            # Set to end value
            apply(self.function, [self.toData] + self.extraArgs)
        elif self.duration == 0.0:
            # Zero duration, just use endpoint
            apply(self.function, [self.toData] + self.extraArgs)
        else:
            # In the middle of the lerp, compute appropriate blended value
            bt = self.blendType(t/self.duration)
            data = (self.fromData * (1 - bt)) + (self.toData * bt)
            # Evaluate function
            apply(self.function, [data] + self.extraArgs)
        # Print debug information
        self.notify.debug('updateFunc() - %s: t = %f' % (self.name, t))
            
    def getBlend(self, blendType):
        """__getBlend(self, string)
        Return the C++ blend class corresponding to blendType string
        """
        # Note, this is temporary until blend functions get exposed
        if (blendType == "easeIn"):
            return LerpBlendHelpers.easeIn
        elif (blendType == "easeOut"):
            return LerpBlendHelpers.easeOut
        elif (blendType == "easeInOut"):
            return LerpBlendHelpers.easeInOut
        elif (blendType == "noBlend"):
            return LerpBlendHelpers.noBlend
        else:
            raise Exception(
                'Error: LerpInterval.__getBlend: Unknown blend type')

# New interface
class LerpFunc(LerpFunctionInterval):
    def __init__(self, *args, **kw):
        LerpFunctionInterval.__init__(self, *args, **kw)
