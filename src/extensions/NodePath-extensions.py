
    """
    NodePath-extensions module: contains methods to extend functionality
    of the NodePath class
    """

    def id(self):
        """Returns the bottom node's this pointer as a unique id"""
        return self.arc()

    def getName(self):
        """Returns the name of the bottom node if it exists, or <noname>"""
        import NamedNode
        # Initialize to a default value
        name = '<noname>'
        # Get the bottom node
        node = self.node()
        # Is it a named node?, If so, see if it has a name
        if issubclass(node.__class__, NamedNode.NamedNode):
            namedNodeName = node.getName()
            # Is it not zero length?
            if len(namedNodeName) != 0:
                name = namedNodeName
        return name

    def setName(self, name = '<noname>'):
        """Returns the name of the bottom node if it exists, or <noname>"""
        import NamedNode        
        # Get the bottom node
        node = self.node()
        # Is it a named node?, If so, see if it has a name
        if issubclass(node.__class__, NamedNode.NamedNode):
            node.setName(name)

    # For iterating over children
    def getChildrenAsList(self):
        """Converts a node path's child NodePathCollection into a list"""
        if self.isEmpty():
            return []
        else:
            children = self.getChildren()
            childrenList = []
            for childNum in range(self.getNumChildren()):
                childrenList.append(children[childNum])
            return childrenList

    def printChildren(self):
        """Prints out the children of the bottom node of a node path"""
        for child in self.getChildrenAsList():
            print child.getName()

    def toggleVis(self):
        """Toggles visibility of a nodePath"""
        if self.isHidden():
            self.show()
            return 1
        else:
            self.hide()
            return 0
            
    def showSiblings(self):
        """Show all the siblings of a node path"""
        for sib in self.getParent().getChildrenAsList():
            if sib.node() != self.node():
                sib.show()

    def hideSiblings(self):
        """Hide all the siblings of a node path"""
        for sib in self.getParent().getChildrenAsList():
            if sib.node() != self.node():
                sib.hide()

    def showAllDescendants(self):
        """Show the node path and all its children"""
        if self.hasArcs():
            self.show()
        for child in self.getChildrenAsList():
            child.showAllDescendants()

    def isolate(self):
        """Show the node path and hide its siblings"""
        self.showAllDescendants()
        self.hideSiblings()

    def remove(self):
        """Remove a node path from the scene graph"""
        # Send message in case anyone needs to do something
        # before node is deleted
        messenger.send('preRemoveNodePath', [self])
        # Remove nodePath
        self.removeNode()

    def reversels(self):
        """Walk up a tree and print out the path to the root"""
        ancestry = self.getAncestry()
        indentString = ""
        for nodePath in ancestry:
            type = nodePath.node().getType().getName()
            name = nodePath.getName()
            print indentString + type + "  " + name
            indentString = indentString + " "

    def getAncestry(self):
        """Get a list of a node path's ancestors"""
        node = self.node()
        if (self.hasParent()):
            ancestry = self.getParent().getAncestry()
            ancestry.append(self)
            return ancestry
        else:
            return [self]

    def getTightBounds(self):
        import Point3
        v1 = Point3.Point3(0)
        v2 = Point3.Point3(0)
        self.calcTightBounds(v1,v2)
        return v1, v2

    def pprintPos(self, other = None, sd = 2):
        """ Pretty print a node path's pos """
        formatString = '%0.' + '%d' % sd + 'f'
        if other:
            pos = self.getPos(other)
            otherString = other.getName() + ', '
        else:
            pos = self.getPos()
            otherString = ''
        print (self.getName() + '.setPos(' + otherString + 
               formatString % pos[0] + ', ' +
               formatString % pos[1] + ', ' +
               formatString % pos[2] +
               ')\n')

    def pprintHpr(self, other = None, sd = 2):
        """ Pretty print a node path's hpr """
        formatString = '%0.' + '%d' % sd + 'f'
        if other:
            hpr = self.getHpr(other)
            otherString = other.getName() + ', '
        else:
            hpr = self.getHpr()
            otherString = ''
        print (self.getName() + '.setHpr(' + otherString + 
               formatString % hpr[0] + ', ' +
               formatString % hpr[1] + ', ' +
               formatString % hpr[2] +
               ')\n')

    def pprintScale(self, other = None, sd = 2):
        """ Pretty print a node path's scale """
        formatString = '%0.' + '%d' % sd + 'f'
        if other:
            scale = self.getScale(other)
            otherString = other.getName() + ', '
        else:
            scale = self.getScale()
            otherString = ''
        print (self.getName() + '.setScale(' + otherString + 
               formatString % scale[0] + ', ' +
               formatString % scale[1] + ', ' +
               formatString % scale[2] +
               ')\n')

    def pprintPosHpr(self, other = None, sd = 2):
        """ Pretty print a node path's pos and, hpr """
        formatString = '%0.' + '%d' % sd + 'f'
        if other:
            pos = self.getPos(other)
            hpr = self.getHpr(other)
            otherString = other.getName() + ', '
        else:
            pos = self.getPos()
            hpr = self.getHpr()
            otherString = ''
        print (self.getName() + '.setPosHpr(' + otherString + 
               formatString % pos[0] + ', ' +
               formatString % pos[1] + ', ' +
               formatString % pos[2] + ', ' +
               formatString % hpr[0] + ', ' +
               formatString % hpr[1] + ', ' +
               formatString % hpr[2] +
               ')\n')

    def pprintPosHprScale(self, other = None, sd = 2):
        """ Pretty print a node path's pos, hpr, and scale """
        formatString = '%0.' + '%d' % sd + 'f'
        if other:
            pos = self.getPos(other)
            hpr = self.getHpr(other)
            scale = self.getScale(other)
            otherString = other.getName() + ', '
        else:
            pos = self.getPos()
            hpr = self.getHpr()
            scale = self.getScale()
            otherString = ''
        print (self.getName() + '.setPosHprScale(' + otherString + 
               formatString % pos[0] + ', ' +
               formatString % pos[1] + ', ' +
               formatString % pos[2] + ', ' +
               formatString % hpr[0] + ', ' +
               formatString % hpr[1] + ', ' +
               formatString % hpr[2] + ', ' +
               formatString % scale[0] + ', ' +
               formatString % scale[1] + ', ' +
               formatString % scale[2] +
               ')\n')

    def iPos(self, other = None):
        """ Set node path's pos to 0,0,0 """
        if other:
            self.setPos(other, 0,0,0)
        else:
            self.setPos(0,0,0)

    def iHpr(self, other = None):
        """ Set node path's hpr to 0,0,0 """
        if other:
            self.setHpr(other, 0,0,0)
        else:
            self.setHpr(0,0,0)

    def iScale(self, other = None):
        """ SEt node path's scale to 1,1,1 """
        if other:
            self.setScale(other, 1,1,1)
        else:
            self.setScale(1,1,1)

    def iPosHpr(self, other = None):
        """ Set node path's pos and hpr to 0,0,0 """
        if other:
            self.setPosHpr(other,0,0,0,0,0,0)
        else:
            self.setPosHpr(0,0,0,0,0,0)

    def iPosHprScale(self, other = None):
        """ Set node path's pos and hpr to 0,0,0 and scale to 1,1,1 """
        if other:
            self.setPosHprScale(other, 0,0,0,0,0,0,1,1,1)
        else:
            self.setPosHprScale(0,0,0,0,0,0,1,1,1)

    # private methods
    
    def __getBlend(self, blendType):
        """__getBlend(self, string)
        Return the C++ blend class corresponding to blendType string
        """
        import LerpBlendHelpers

        if (blendType == "easeIn"):
            return LerpBlendHelpers.easeIn
        elif (blendType == "easeOut"):
            return LerpBlendHelpers.easeOut
        elif (blendType == "easeInOut"):
            return LerpBlendHelpers.easeInOut
        elif (blendType == "noBlend"):
            return LerpBlendHelpers.noBlend
        else:
            raise Exception("Error: NodePath.__getBlend: Unknown blend type")

            
    def __lerp(self, functorFunc, duration, blendType, taskName=None):
        """
        __lerp(self, functorFunc, float, string, string)
        Basic lerp functionality used by other lerps.
        Fire off a lerp. Make it a task if taskName given.
        """
        # functorFunc is a function which can be called to create a functor.
        # functor creation is defered so initial state (sampled in functorFunc)
        # will be appropriate for the time the lerp is spawned
        import Task
        from TaskManagerGlobal import taskMgr
        
        # upon death remove the functorFunc
        def lerpUponDeath(task):
            # Try to break circular references
            try:
                del task.functorFunc
            except:
                pass
            try:
                del task.lerp
            except:
                pass
        
        # make the task function
        def lerpTaskFunc(task):
            from Lerp import Lerp
            from ClockObject import ClockObject
            from Task import Task, cont, done
            if task.init == 1:
                # make the lerp
                functor = task.functorFunc()
                task.lerp = Lerp(functor, task.duration, task.blendType)
                task.init = 0
            dt = ClockObject.getGlobalClock().getDt()
            task.lerp.setStepSize(dt)
            task.lerp.step()
            if (task.lerp.isDone()):
                # Reset the init flag, in case the task gets re-used
                task.init = 1
                return(done)
            else:
                return(cont)
        
        # make the lerp task
        lerpTask = Task.Task(lerpTaskFunc)
        lerpTask.init = 1
        lerpTask.functorFunc = functorFunc
        lerpTask.duration = duration
        lerpTask.blendType = self.__getBlend(blendType)
        lerpTask.uponDeath = lerpUponDeath
        
        if (taskName == None):
            # don't spawn a task, return one instead
            return lerpTask
        else:
            # spawn the lerp task
            taskMgr.add(lerpTask, taskName)
            return lerpTask

    def __autoLerp(self, functorFunc, time, blendType, taskName):
        """_autoLerp(self, functor, float, string, string)
        This lerp uses C++ to handle the stepping. Bonus is
        its more efficient, trade-off is there is less control"""
        import AutonomousLerp
        # make a lerp that lives in C++ land
        functor = functorFunc()
        lerp = AutonomousLerp.AutonomousLerp(functor, time,
                              self.__getBlend(blendType),
                              base.eventHandler)
        lerp.start()
        return lerp


    # user callable lerp methods
    def lerpColor(self, *posArgs, **keyArgs):
        """lerpColor(self, *positionArgs, **keywordArgs)
        determine which lerpColor* to call based on arguments
        """
        if (len(posArgs) == 2):
            return apply(self.lerpColorVBase4, posArgs, keyArgs)
        elif (len(posArgs) == 3):
            return apply(self.lerpColorVBase4VBase4, posArgs, keyArgs)
        elif (len(posArgs) == 5):
            return apply(self.lerpColorRGBA, posArgs, keyArgs)
        elif (len(posArgs) == 9):
            return apply(self.lerpColorRGBARGBA, posArgs, keyArgs)
        else:
            # bad args
            raise Exception("Error: NodePath.lerpColor: bad number of args")

            
    def lerpColorRGBA(self, r, g, b, a, time,
                      blendType="noBlend", auto=None, task=None):
        """lerpColorRGBA(self, float, float, float, float, float,
        string="noBlend", string=none, string=none)
        """
        def functorFunc(self = self, r = r, g = g, b = b, a = a):
            import ColorLerpFunctor
            # just end rgba values, use current color rgba values for start
            startColor = self.getColor()
            functor = ColorLerpFunctor.ColorLerpFunctor(
                self,
                startColor[0], startColor[1],
                startColor[2], startColor[3],
                r, g, b, a)
            return functor
        #determine whether to use auto, spawned, or blocking lerp
        if (auto != None):
            return self.__autoLerp(functorFunc, time, blendType, auto)
        elif (task != None):
            return self.__lerp(functorFunc, time, blendType, task)
        else:
            return self.__lerp(functorFunc, time, blendType)

    def lerpColorRGBARGBA(self, sr, sg, sb, sa, er, eg, eb, ea, time,
                          blendType="noBlend", auto=None, task=None):
        """lerpColorRGBARGBA(self, float, float, float, float, float,
        float, float, float, float, string="noBlend", string=none, string=none)
        """
        def functorFunc(self = self, sr = sr, sg = sg, sb = sb, sa = sa,
                        er = er, eg = eg, eb = eb, ea = ea):
            import ColorLerpFunctor
            # start and end rgba values
            functor = ColorLerpFunctor.ColorLerpFunctor(self, sr, sg, sb, sa,
                                                        er, eg, eb, ea)
            return functor
        #determine whether to use auto, spawned, or blocking lerp
        if (auto != None):
            return self.__autoLerp(functorFunc, time, blendType, auto)
        elif (task != None):
            return self.__lerp(functorFunc, time, blendType, task)
        else:
            return self.__lerp(functorFunc, time, blendType)

    def lerpColorVBase4(self, endColor, time,
                        blendType="noBlend", auto=None, task=None):
        """lerpColorVBase4(self, VBase4, float, string="noBlend", string=none,
        string=none)
        """
        def functorFunc(self = self, endColor = endColor):
            import ColorLerpFunctor
            # just end vec4, use current color for start
            startColor = self.getColor()
            functor = ColorLerpFunctor.ColorLerpFunctor(
                self, startColor, endColor)
            return functor
        #determine whether to use auto, spawned, or blocking lerp
        if (auto != None):
            return self.__autoLerp(functorFunc, time, blendType, auto)
        elif (task != None):
            return self.__lerp(functorFunc, time, blendType, task)
        else:
            return self.__lerp(functorFunc, time, blendType)

    def lerpColorVBase4VBase4(self, startColor, endColor, time,
                          blendType="noBlend", auto=None, task=None):
        """lerpColorVBase4VBase4(self, VBase4, VBase4, float, string="noBlend",
        string=none, string=none)
        """
        def functorFunc(self = self, startColor = startColor,
                        endColor = endColor):
            import ColorLerpFunctor
            # start color and end vec
            functor = ColorLerpFunctor.ColorLerpFunctor(
                self, startColor, endColor)
            return functor
        #determine whether to use auto, spawned, or blocking lerp
        if (auto != None):
            return self.__autoLerp(functorFunc, time, blendType, auto)
        elif (task != None):
            return self.__lerp(functorFunc, time, blendType, task)
        else:
            return self.__lerp(functorFunc, time, blendType)
            

    def lerpHpr(self, *posArgs, **keyArgs):
        """lerpHpr(self, *positionArgs, **keywordArgs)
        Determine whether to call lerpHprHPR or lerpHprVBase3
        based on first argument
        """
        # check to see if lerping with
        # three floats or a VBase3
        if (len(posArgs) == 4):
            return apply(self.lerpHprHPR, posArgs, keyArgs)
        elif(len(posArgs) == 2):
            return apply(self.lerpHprVBase3, posArgs, keyArgs)
        else:
            # bad args
            raise Exception("Error: NodePath.lerpHpr: bad number of args")
    
    def lerpHprHPR(self, h, p, r, time, other=None,
                   blendType="noBlend", auto=None, task=None, shortest=1):
        """lerpHprHPR(self, float, float, float, float, string="noBlend",
        string=none, string=none, NodePath=none)
        Perform a hpr lerp with three floats as the end point
        """
        def functorFunc(self = self, h = h, p = p, r = r,
                        other = other, shortest=shortest):
            import HprLerpFunctor
            # it's individual hpr components
            if (other != None):
                # lerp wrt other
                startHpr = self.getHpr(other)
                functor = HprLerpFunctor.HprLerpFunctor(
                    self,
                    startHpr[0], startHpr[1], startHpr[2],
                    h, p, r, other)
                if shortest:
                    functor.takeShortest()
            else:
                startHpr = self.getHpr()
                functor = HprLerpFunctor.HprLerpFunctor(
                    self,
                    startHpr[0], startHpr[1], startHpr[2],
                    h, p, r)
                if shortest:
                    functor.takeShortest()
            return functor
        #determine whether to use auto, spawned, or blocking lerp
        if (auto != None):
            return self.__autoLerp(functorFunc, time, blendType, auto)
        elif (task != None):
            return self.__lerp(functorFunc, time, blendType, task)
        else:
            return self.__lerp(functorFunc, time, blendType)
    
    def lerpHprVBase3(self, hpr, time, other=None,
                      blendType="noBlend", auto=None, task=None, shortest=1):
        """lerpHprVBase3(self, VBase3, float, string="noBlend", string=none,
        string=none, NodePath=None)
        Perform a hpr lerp with a VBase3 as the end point
        """
        def functorFunc(self = self, hpr = hpr,
                        other = other, shortest=shortest):
            import HprLerpFunctor
            # it's a vbase3 hpr
            if (other != None):
                # lerp wrt other
                functor = HprLerpFunctor.HprLerpFunctor(
                    self, (self.getHpr(other)), hpr, other)
                if shortest:
                    functor.takeShortest()
            else:
                functor = HprLerpFunctor.HprLerpFunctor(
                    self, (self.getHpr()), hpr)
                if shortest:
                    functor.takeShortest()
            return functor
        #determine whether to use auto, spawned, or blocking lerp
        if (auto != None):
            return self.__autoLerp(functorFunc, time, blendType, auto)
        elif (task != None):
            return self.__lerp(functorFunc, time, blendType, task)
        else:
            return self.__lerp(functorFunc, time, blendType)
        

    def lerpPos(self, *posArgs, **keyArgs):
        """lerpPos(self, *positionArgs, **keywordArgs)
        Determine whether to call lerpPosXYZ or lerpPosPoint3
        based on the first argument
        """
        # check to see if lerping with three
        # floats or a Point3
        if (len(posArgs) == 4):
            return apply(self.lerpPosXYZ, posArgs, keyArgs)
        elif(len(posArgs) == 2):
            return apply(self.lerpPosPoint3, posArgs, keyArgs)
        else:
            # bad number off args
            raise Exception("Error: NodePath.lerpPos: bad number of args")

    def lerpPosXYZ(self, x, y, z, time, other=None,
                   blendType="noBlend", auto=None, task=None):
        """lerpPosXYZ(self, float, float, float, float, string="noBlend",
        string=None, NodePath=None)
        Perform a pos lerp with three floats as the end point
        """
        def functorFunc(self = self, x = x, y = y, z = z, other = other):
            import PosLerpFunctor
            if (other != None):
                # lerp wrt other
                startPos = self.getPos(other)
                functor = PosLerpFunctor.PosLerpFunctor(self,
                                         startPos[0], startPos[1], startPos[2],
                                         x, y, z, other)
            else:
                startPos = self.getPos()
                functor = PosLerpFunctor.PosLerpFunctor(self, startPos[0],
                                         startPos[1], startPos[2], x, y, z)
            return functor
        #determine whether to use auto, spawned, or blocking lerp
        if (auto != None):
            return  self.__autoLerp(functorFunc, time, blendType, auto)
        elif (task != None):
            return self.__lerp(functorFunc, time, blendType, task)
        else:
            return self.__lerp(functorFunc, time, blendType)

    def lerpPosPoint3(self, pos, time, other=None,
                      blendType="noBlend", auto=None, task=None):
        """lerpPosPoint3(self, Point3, float, string="noBlend", string=None,
        string=None, NodePath=None)
        Perform a pos lerp with a Point3 as the end point
        """
        def functorFunc(self = self, pos = pos, other = other):
            import PosLerpFunctor
            if (other != None):
                #lerp wrt other
                functor = PosLerpFunctor.PosLerpFunctor(
                    self, (self.getPos(other)), pos, other)
            else:
                functor = PosLerpFunctor.PosLerpFunctor(
                    self, (self.getPos()), pos)
            return functor
        #determine whether to use auto, spawned, or blocking lerp
        if (auto != None):
            return self.__autoLerp(functorFunc, time, blendType, auto)
        elif (task != None):
            return self.__lerp(functorFunc, time, blendType, task)
        else:
            return self.__lerp(functorFunc, time, blendType)


    def lerpPosHpr(self, *posArgs, **keyArgs):
        """lerpPosHpr(self, *positionArgs, **keywordArgs)
        Determine whether to call lerpPosHprXYZHPR or lerpHprPoint3VBase3
        based on first argument
        """
        # check to see if lerping with
        # six floats or a Point3 and a VBase3
        if (len(posArgs) == 7):
            return apply(self.lerpPosHprXYZHPR, posArgs, keyArgs)
        elif(len(posArgs) == 3):
            return apply(self.lerpPosHprPoint3VBase3, posArgs, keyArgs)
        else:
            # bad number off args
            raise Exception("Error: NodePath.lerpPosHpr: bad number of args")

    def lerpPosHprPoint3VBase3(self, pos, hpr, time, other=None,
                               blendType="noBlend", auto=None, task=None, shortest=1):
        """lerpPosHprPoint3VBase3(self, Point3, VBase3, string="noBlend",
        string=none, string=none, NodePath=None)
        """
        def functorFunc(self = self, pos = pos, hpr = hpr,
                        other = other, shortest=shortest):
            import PosHprLerpFunctor
            if (other != None):
                # lerp wrt other
                startPos = self.getPos(other)
                startHpr = self.getHpr(other)
                functor = PosHprLerpFunctor.PosHprLerpFunctor(
                    self, startPos, pos,
                    startHpr, hpr, other)
                if shortest:
                    functor.takeShortest()
            else:
                startPos = self.getPos()
                startHpr = self.getHpr()
                functor = PosHprLerpFunctor.PosHprLerpFunctor(
                    self, startPos, pos,
                    startHpr, hpr)
                if shortest:
                    functor.takeShortest()
            return functor
        #determine whether to use auto, spawned, or blocking lerp
        if (auto != None):
            return self.__autoLerp(functorFunc, time, blendType, auto)
        elif (task != None):
            return self.__lerp(functorFunc, time, blendType, task)
        else:
            return self.__lerp(functorFunc, time, blendType)

    def lerpPosHprXYZHPR(self, x, y, z, h, p, r, time, other=None,
                         blendType="noBlend", auto=None, task=None, shortest=1):
        """lerpPosHpr(self, float, string="noBlend", string=none,
        string=none, NodePath=None)
        """
        def functorFunc(self = self, x = x, y = y, z = z,
                        h = h, p = p, r = r, other = other, shortest=shortest):
            import PosHprLerpFunctor
            if (other != None):
                # lerp wrt other
                startPos = self.getPos(other)
                startHpr = self.getHpr(other)
                functor = PosHprLerpFunctor.PosHprLerpFunctor(self,
                                            startPos[0], startPos[1],
                                            startPos[2], x, y, z,
                                            startHpr[0], startHpr[1],
                                            startHpr[2], h, p, r,
                                            other)
                if shortest:
                    functor.takeShortest()
            else:
                startPos = self.getPos()
                startHpr = self.getHpr()
                functor = PosHprLerpFunctor.PosHprLerpFunctor(self,
                                            startPos[0], startPos[1],
                                            startPos[2], x, y, z,
                                            startHpr[0], startHpr[1],
                                            startHpr[2], h, p, r)
                if shortest:
                    functor.takeShortest()
            return functor
        #determine whether to use auto, spawned, or blocking lerp
        if (auto != None):
            return self.__autoLerp(functorFunc, time, blendType, auto)
        elif (task != None):
            return self.__lerp(functorFunc, time, blendType, task)
        else:
            return self.__lerp(functorFunc, time, blendType)


    def lerpPosHprScale(self, pos, hpr, scale, time, other=None,
                        blendType="noBlend", auto=None, task=None, shortest=1):
        """lerpPosHpr(self, Point3, VBase3, float, float, string="noBlend",
        string=none, string=none, NodePath=None)
        Only one case, no need for extra args. Call the appropriate lerp
        (auto, spawned, or blocking) based on how(if) a task name is given
        """
        def functorFunc(self = self, pos = pos, hpr = hpr,
                        scale = scale, other = other, shortest=shortest):
            import PosHprScaleLerpFunctor
            if (other != None):
                # lerp wrt other
                startPos = self.getPos(other)
                startHpr = self.getHpr(other)
                startScale = self.getScale(other)
                functor = PosHprScaleLerpFunctor.PosHprScaleLerpFunctor(self,
                                                 startPos, pos,
                                                 startHpr, hpr,
                                                 startScale, scale, other)
                if shortest:
                    functor.takeShortest()
            else:
                startPos = self.getPos()
                startHpr = self.getHpr()
                startScale = self.getScale()
                functor = PosHprScaleLerpFunctor.PosHprScaleLerpFunctor(self,
                                                 startPos, pos,
                                                 startHpr, hpr,
                                                 startScale, scale)
                if shortest:
                    functor.takeShortest()
            return functor
        #determine whether to use auto, spawned, or blocking lerp
        if (auto != None):
            return self.__autoLerp(functorFunc, time, blendType, auto)
        elif (task != None):
            return self.__lerp(functorFunc, time, blendType, task)
        else:
            return self.__lerp(functorFunc, time, blendType)


    def lerpScale(self, *posArgs, **keyArgs):
        """lerpSclae(self, *positionArgs, **keywordArgs)
        Determine whether to call lerpScaleXYZ or lerpScaleaseV3
        based on the first argument
        """
        # check to see if lerping with three
        # floats or a Point3
        if (len(posArgs) == 4):
            return apply(self.lerpScaleXYZ, posArgs, keyArgs)
        elif(len(posArgs) == 2):
            return apply(self.lerpScaleVBase3, posArgs, keyArgs)
        else:
            # bad number off args
            raise Exception("Error: NodePath.lerpScale: bad number of args")

    def lerpScaleVBase3(self, scale, time, other=None,
                        blendType="noBlend", auto=None, task=None):
        """lerpPos(self, VBase3, float, string="noBlend", string=none,
        string=none, NodePath=None)
        """
        def functorFunc(self = self, scale = scale, other = other):
            import ScaleLerpFunctor
            if (other != None):
                # lerp wrt other
                functor = ScaleLerpFunctor.ScaleLerpFunctor(self,
                                           (self.getScale(other)),
                                           scale, other)
            else:
                functor = ScaleLerpFunctor.ScaleLerpFunctor(self,
                                           (self.getScale()), scale)

            return functor
        #determine whether to use auto, spawned, or blocking lerp
        if (auto != None):
            return self.__autoLerp(functorFunc, time, blendType, auto)
        elif (task != None):
            return self.__lerp(functorFunc, time, blendType, task)
        else:
            return self.__lerp(functorFunc, time, blendType)

    def lerpScaleXYZ(self, sx, sy, sz, time, other=None,
                     blendType="noBlend", auto=None, task=None):
        """lerpPos(self, float, float, float, float, string="noBlend",
        string=none, string=none, NodePath=None)
        """
        def functorFunc(self = self, sx = sx, sy = sy, sz = sz, other = other):
            import ScaleLerpFunctor
            if (other != None):
                # lerp wrt other
                startScale = self.getScale(other)
                functor = ScaleLerpFunctor.ScaleLerpFunctor(self,
                                           startScale[0], startScale[1],
                                           startScale[2], sx, sy, sz, other)
            else:
                startScale = self.getScale()
                functor = ScaleLerpFunctor.ScaleLerpFunctor(self,
                                           startScale[0], startScale[1],
                                           startScale[2], sx, sy, sz)
            return functor
        #determine whether to use auto, spawned, or blocking lerp
        if (auto != None):
            return self.__autoLerp(functorFunc, time, blendType, auto)
        elif (task != None):
            return self.__lerp(functorFunc, time, blendType, task)
        else:
            return self.__lerp(functorFunc, time, blendType)
            
    def place(self):
        base.wantTk = 1
        base.wantDIRECT = 1
        import DirectSession
        direct.enable()
        import Placer
        return Placer.place(self)

    def explore(self):
        base.wantTk = 1
        base.wantDIRECT = 1
        import TkGlobal
        import SceneGraphExplorer
        return SceneGraphExplorer.explore(self)

    def rgbPanel(self, cb = None):
        base.wantTk = 1
        base.wantDIRECT = 1
        import TkGlobal
        import Slider
        return Slider.rgbPanel(self, cb)

    def select(self):
        base.wantTk = 1
        base.wantDIRECT = 1
        import DirectSession
        direct.select(self)

    def deselect(self):
        base.wantTk = 1
        base.wantDIRECT = 1
        import DirectSession
        direct.deselect(self)

    def setAlphaScale(self, alpha):
        self.setColorScale(1, 1, 1, alpha)

    def setAllColorScale(self, color):
        self.setColorScale(color, color, color, 1)

    def showCS(self, mask = None):
        """showCS(self, mask)
        Shows the collision solids at or below this node.  If mask is
        not None, it is a BitMask32 object (e.g. WallBitmask,
        CameraBitmask) that indicates which particular collision
        solids should be made visible; otherwise, all of them will be.
        """
        npc = self.findAllMatches('**/+CollisionNode')
        for p in range(0, npc.getNumPaths()):
            np = npc[p]
            if (mask == None or (np.node().getIntoCollideMask() & mask).getWord()):
                np.show()

    def hideCS(self, mask = None):
        """hideCS(self, mask)
        Hides the collision solids at or below this node.  If mask is
        not None, it is a BitMask32 object (e.g. WallBitmask,
        CameraBitmask) that indicates which particular collision
        solids should be hidden; otherwise, all of them will be.
        """
        npc = self.findAllMatches('**/+CollisionNode')
        for p in range(0, npc.getNumPaths()):
            np = npc[p]
            if (mask == None or (np.node().getIntoCollideMask() & mask).getWord()):
                np.hide()

    def posInterval(self, *args, **kw):
        import LerpInterval
        return LerpInterval.LerpPosInterval(self, *args, **kw)
    
    def hprInterval(self, *args, **kw):
        import LerpInterval
        return LerpInterval.LerpHprInterval(self, *args, **kw)

    def scaleInterval(self, *args, **kw):
        import LerpInterval
        return LerpInterval.LerpScaleInterval(self, *args, **kw)

    def posHprInterval(self, *args, **kw):
        import LerpInterval
        return LerpInterval.LerpPosHprInterval(self, *args, **kw)

    def hprScaleInterval(self, *args, **kw):
        import LerpInterval
        return LerpInterval.LerpHprScaleInterval(self, *args, **kw)

    def posHprScaleInterval(self, *args, **kw):
        import LerpInterval
        return LerpInterval.LerpPosHprScaleInterval(self, *args, **kw)

    def colorInterval(self, *args, **kw):
        import LerpInterval
        return LerpInterval.LerpColorInterval(self, *args, **kw)

    def colorScaleInterval(self, *args, **kw):
        import LerpInterval
        return LerpInterval.LerpColorScaleInterval(self, *args, **kw)
