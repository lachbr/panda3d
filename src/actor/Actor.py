"""Actor module: contains the Actor class"""

from pandac.PandaModules import *
from direct.showbase.DirectObject import DirectObject
from pandac.PandaModules import LODNode
import types
import copy as copy_module

class Actor(DirectObject, NodePath):
    """
    Actor class: Contains methods for creating, manipulating
    and playing animations on characters
    """
    notify = directNotify.newCategory("Actor")
    partPrefix = "__Actor_"

    modelLoaderOptions = LoaderOptions(LoaderOptions.LFSearch |
                                       LoaderOptions.LFReportErrors |
                                       LoaderOptions.LFConvertSkeleton)
    animLoaderOptions =  LoaderOptions(LoaderOptions.LFSearch |
                                       LoaderOptions.LFReportErrors |
                                       LoaderOptions.LFConvertAnim)

    def __init__(self, models=None, anims=None, other=None, copy=1):
        """__init__(self, string | string:string{}, string:string{} |
        string:(string:string{}){}, Actor=None)
        Actor constructor: can be used to create single or multipart
        actors. If another Actor is supplied as an argument this
        method acts like a copy constructor. Single part actors are
        created by calling with a model and animation dictionary
        (animName:animPath{}) as follows:

           a = Actor("panda-3k.egg", {"walk":"panda-walk.egg" \
                                      "run":"panda-run.egg"})

        This could be displayed and animated as such:

           a.reparentTo(render)
           a.loop("walk")
           a.stop()

        Multipart actors expect a dictionary of parts and a dictionary
        of animation dictionaries (partName:(animName:animPath{}){}) as
        below:

            a = Actor(

                # part dictionary
                {"head":"char/dogMM/dogMM_Shorts-head-mod", \
                 "torso":"char/dogMM/dogMM_Shorts-torso-mod", \
                 "legs":"char/dogMM/dogMM_Shorts-legs-mod"}, \

                # dictionary of anim dictionaries
                {"head":{"walk":"char/dogMM/dogMM_Shorts-head-walk", \
                         "run":"char/dogMM/dogMM_Shorts-head-run"}, \
                 "torso":{"walk":"char/dogMM/dogMM_Shorts-torso-walk", \
                          "run":"char/dogMM/dogMM_Shorts-torso-run"}, \
                 "legs":{"walk":"char/dogMM/dogMM_Shorts-legs-walk", \
                         "run":"char/dogMM/dogMM_Shorts-legs-run"} \
                 })

        In addition multipart actor parts need to be connected together
        in a meaningful fashion:

            a.attach("head", "torso", "joint-head")
            a.attach("torso", "legs", "joint-hips")

        #
        # ADD LOD COMMENT HERE!
        #

        Other useful Actor class functions:

            #fix actor eye rendering
            a.drawInFront("joint-pupil?", "eyes*")

            #fix bounding volumes - this must be done after drawing
            #the actor for a few frames, otherwise it has no effect
            a.fixBounds()
        """
        try:
            self.Actor_initialized
            return
        except:
            self.Actor_initialized = 1

        # initialize our NodePath essence
        NodePath.__init__(self)

        self.__autoCopy = copy

        # create data structures
        self.__partBundleDict = {}
        self.__subpartDict = {}
        self.__sortedLODNames = []
        self.__animControlDict = {}
        self.__controlJoints = {}

        self.__LODNode = None
        self.switches = None

        if (other == None):
            # act like a normal constructor

            # create base hierarchy
            self.gotName = 0
            root = ModelNode('actor')
            root.setPreserveTransform(1)
            self.assign(NodePath(root))
            self.setGeomNode(self.attachNewNode(ModelNode('actorGeom')))
            self.__hasLOD = 0

            # load models
            #
            # four cases:
            #
            #   models, anims{} = single part actor
            #   models{}, anims{} =  single part actor w/ LOD
            #   models{}, anims{}{} = multi-part actor
            #   models{}{}, anims{}{} = multi-part actor w/ LOD
            #
            # make sure we have models
            if (models):
                # do we have a dictionary of models?
                if (type(models)==type({})):
                    # if this is a dictionary of dictionaries
                    if (type(models[models.keys()[0]]) == type({})):
                        # then it must be a multipart actor w/LOD
                        self.setLODNode()
                        # preserve numerical order for lod's
                        # this will make it easier to set ranges
                        sortedKeys = models.keys()
                        sortedKeys.sort()
                        for lodName in sortedKeys:
                            # make a node under the LOD switch
                            # for each lod (just because!)
                            self.addLOD(str(lodName))
                            # iterate over both dicts
                            for modelName in models[lodName].keys():
                                self.loadModel(models[lodName][modelName],
                                               modelName, lodName, copy = copy)
                    # then if there is a dictionary of dictionaries of anims
                    elif (type(anims[anims.keys()[0]])==type({})):
                        # then this is a multipart actor w/o LOD
                        for partName in models.keys():
                            # pass in each part
                            self.loadModel(models[partName], partName, copy = copy)
                    else:
                        # it is a single part actor w/LOD
                        self.setLODNode()
                        # preserve order of LOD's
                        sortedKeys = models.keys()
                        sortedKeys.sort()
                        for lodName in sortedKeys:
                            self.addLOD(str(lodName))
                            # pass in dictionary of parts
                            self.loadModel(models[lodName], lodName=lodName, copy = copy)
                else:
                    # else it is a single part actor
                    self.loadModel(models, copy = copy)

            # load anims
            # make sure the actor has animations
            if (anims):
                if (len(anims) >= 1):
                    # if so, does it have a dictionary of dictionaries?
                    if (type(anims[anims.keys()[0]])==type({})):
                        # are the models a dict of dicts too?
                        if (type(models)==type({})):
                            if (type(models[models.keys()[0]]) == type({})):
                                # then we have a multi-part w/ LOD
                                sortedKeys = models.keys()
                                sortedKeys.sort()
                                for lodName in sortedKeys:
                                    # iterate over both dicts
                                    for partName in anims.keys():
                                        self.loadAnims(
                                            anims[partName], partName, lodName)
                            else:
                                # then it must be multi-part w/o LOD
                                for partName in anims.keys():
                                    self.loadAnims(anims[partName], partName)
                    elif (type(models)==type({})):
                        # then we have single-part w/ LOD
                        sortedKeys = models.keys()
                        sortedKeys.sort()
                        for lodName in sortedKeys:
                            self.loadAnims(anims, lodName=lodName)
                    else:
                        # else it is single-part w/o LOD
                        self.loadAnims(anims)

        else:
            self.copyActor(other, True) # overwrite everything

        # For now, all Actors will by default set their top bounding
        # volume to be the "final" bounding volume: the bounding
        # volumes below the top volume will not be tested.  If a cull
        # test passes the top bounding volume, the whole Actor is
        # rendered.

        # We do this partly because an Actor is likely to be a fairly
        # small object relative to the scene, and is pretty much going
        # to be all onscreen or all offscreen anyway; and partly
        # because of the Character bug that doesn't update the
        # bounding volume for pieces that animate away from their
        # original position.  It's disturbing to see someone's hands
        # disappear; better to cull the whole object or none of it.
        self.__geomNode.node().setFinal(1)

    def delete(self):
        try:
            self.Actor_deleted
            return
        except:
            self.Actor_deleted = 1
            self.cleanup()

    def copyActor(self, other, overwrite=False):
            # act like a copy constructor
            self.gotName = other.gotName

            # copy the scene graph elements of other
            if (overwrite):
                otherCopy = other.copyTo(hidden)
                otherCopy.detachNode()
                # assign these elements to ourselve (overwrite)
                self.assign(otherCopy)
            else:
                # just copy these to ourselve
                otherCopy = other.copyTo(self)
            self.setGeomNode(otherCopy.getChild(0))

            # copy the part dictionary from other
            self.__copyPartBundles(other)
            self.__subpartDict = copy_module.deepcopy(other.__subpartDict)

            # copy the anim dictionary from other
            self.__copyAnimControls(other)

            # copy the switches for lods
            self.switches = other.switches
            self.__LODNode = self.find('**/+LODNode')
            self.__hasLOD = 0
            if (not self.__LODNode.isEmpty()):
                self.__hasLOD = 1

    def __cmp__(self, other):
        # Actor inherits from NodePath, which inherits a definition of
        # __cmp__ from FFIExternalObject that uses the NodePath's
        # compareTo() method to compare different NodePaths.  But we
        # don't want this behavior for Actors; Actors should only be
        # compared pointerwise.  A NodePath that happens to reference
        # the same node is still different from the Actor.
        if self is other:
            return 0
        else:
            return 1

    def __str__(self):
        """
        Actor print function
        """
        return "Actor: partBundleDict = %s,\n animControlDict = %s" % \
               (self.__partBundleDict, self.__animControlDict)

    def listJoints(self, partName="modelRoot", lodName="lodRoot"):
        """Handy utility function to list the joint hierarchy of the
        actor. """

        partBundleDict = self.__partBundleDict.get(lodName)
        if not partBundleDict:
            Actor.notify.error("no lod named: %s" % (lodName))

        truePartName = partName
        subset = PartSubset()
        subpartDef = self.__subpartDict.get(partName)
        if subpartDef:
            truePartName, subset = subpartDef

        bundle = partBundleDict.get(truePartName)
        if bundle == None:
            Actor.notify.error("no part named: %s" % (partName))

        self.__doListJoints(0, bundle.node().getBundle(),
                            subset.isIncludeEmpty(), subset)

    def __doListJoints(self, indentLevel, part, isIncluded, subset):
        name = part.getName()
        if subset.matchesInclude(name):
            isIncluded = True
        elif subset.matchesExclude(name):
            isIncluded = False

        if isIncluded:
            value = ''
            if hasattr(part, 'outputValue'):
                lineStream = LineStream.LineStream()
                part.outputValue(lineStream)
                value = lineStream.getLine()

            print ' ' * indentLevel, part.getName(), value

        for i in range(part.getNumChildren()):
            self.__doListJoints(indentLevel + 2, part.getChild(i),
                                isIncluded, subset)


    def getActorInfo(self):
        """
        Utility function to create a list of information about an actor.
        Useful for iterating over details of an actor.
        """
        lodInfo = []
        for lodName in self.__animControlDict.keys():
            partDict = self.__animControlDict[lodName]
            partInfo = []
            for partName in partDict.keys():
                truePartName = self.__subpartDict.get(partName, [partName])[0]
                partBundle = self.__partBundleDict[lodName][truePartName]
                animDict = partDict[partName]
                animInfo = []
                for animName in animDict.keys():
                    file = animDict[animName][0]
                    animControl = animDict[animName][1]
                    animInfo.append([animName, file, animControl])
                partInfo.append([partName, partBundle, animInfo])
            lodInfo.append([lodName, partInfo])
        return lodInfo

    def getAnimNames(self):
        animNames = []
        for lodName, lodInfo in self.getActorInfo():
            for partName, bundle, animInfo in lodInfo:
                for animName, file, animControl in animInfo:
                    if animName not in animNames:
                        animNames.append(animName)
        return animNames

    def pprint(self):
        """
        Pretty print actor's details
        """
        for lodName, lodInfo in self.getActorInfo():
            print 'LOD:', lodName
            for partName, bundle, animInfo in lodInfo:
                print '  Part:', partName
                print '  Bundle:', `bundle`
                for animName, file, animControl in animInfo:
                    print '    Anim:', animName
                    print '      File:', file
                    if animControl == None:
                        print ' (not loaded)'
                    else:
                        print ('      NumFrames: %d PlayRate: %0.2f' %
                               (animControl.getNumFrames(),
                                animControl.getPlayRate()))

    def cleanup(self):
        """
        Actor cleanup function
        """
        self.stop()
        self.flush()
        self.__geomNode.removeNode()
        if not self.isEmpty():
            self.removeNode()

    def flush(self):
        """
        Actor flush function
        """
        self.__partBundleDict = {}
        self.__subpartDict = {}
        self.__sortedLODNames = []
        self.__animControlDict = {}
        self.__controlJoints = {}

        if self.__LODNode and (not self.__LODNode.isEmpty()):
            self.__LODNode.removeNode()
            self.__LODNode = None

        self.__hasLOD = 0

    # accessing

    def getAnimControlDict(self):
        return self.__animControlDict

    def getPartBundleDict(self):
        return self.__partBundleDict

    def __updateSortedLODNames(self):
        # Cache the sorted LOD names so we dont have to grab them
        # and sort them every time somebody asks for the list
        self.__sortedLODNames = self.__partBundleDict.keys()
        # Reverse sort the doing a string->int
        def sortFunc(x, y):
            if not str(x).isdigit():
                smap = {'h':3,
                        'm':2,
                        'l':1,
                        'f':0}
                return cmp(smap[y[0]], smap[x[0]])
            else:
                return cmp (int(y), int(x))

        self.__sortedLODNames.sort(sortFunc)

    def getLODNames(self):
        """
        Return list of Actor LOD names. If not an LOD actor,
        returns 'lodRoot'
        Caution - this returns a reference to the list - not your own copy
        """
        return self.__sortedLODNames

    def getPartNames(self):
        """
        Return list of Actor part names. If not an multipart actor,
        returns 'modelRoot' NOTE: returns parts of arbitrary LOD
        """
        return self.__partBundleDict.values()[0].keys() + self.__subpartDict.keys()


    def getGeomNode(self):
        """
        Return the node that contains all actor geometry
        """
        return self.__geomNode

    def setGeomNode(self, node):
        """
        Set the node that contains all actor geometry
        """
        self.__geomNode = node

    def getLODNode(self):
        """
        Return the node that switches actor geometry in and out"""
        return self.__LODNode.node()

    def setLODNode(self, node=None):
        """
        Set the node that switches actor geometry in and out.
        If one is not supplied as an argument, make one
        """
        if (node == None):
            lod = LODNode("lod")
            self.__LODNode = self.__geomNode.attachNewNode(lod)
        else:
            self.__LODNode = self.__geomNode.attachNewNode(node)
        self.__hasLOD = 1
        self.switches = {}

    def useLOD(self, lodName):
        """
        Make the Actor ONLY display the given LOD
        """
        # make sure we don't call this twice in a row
        # and pollute the the switches dictionary
        self.resetLOD()
        # store the data in the switches for later use
        sortedKeys = self.switches.keys()
        sortedKeys.sort()
        for eachLod in sortedKeys:
            index = sortedKeys.index(eachLod)
            # set the switches to not display ever
            self.__LODNode.node().setSwitch(index, 0, 10000)
        # turn the given LOD on 'always'
        index = sortedKeys.index(lodName)
        self.__LODNode.node().setSwitch(index, 10000, 0)

    def printLOD(self):
        sortedKeys = self.switches.keys()
        sortedKeys.sort()
        for eachLod in sortedKeys:
            print "python switches for %s: in: %d, out %d" % (eachLod,
                                              self.switches[eachLod][0],
                                              self.switches[eachLod][1])

        switchNum = self.__LODNode.node().getNumSwitches()
        for eachSwitch in range(0, switchNum):
            print "c++ switches for %d: in: %d, out: %d" % (eachSwitch,
                   self.__LODNode.node().getIn(eachSwitch),
                   self.__LODNode.node().getOut(eachSwitch))


    def resetLOD(self):
        """
        Restore all switch distance info (usually after a useLOD call)"""
        sortedKeys = self.switches.keys()
        sortedKeys.sort()
        for eachLod in sortedKeys:
            index = sortedKeys.index(eachLod)
            self.__LODNode.node().setSwitch(index, self.switches[eachLod][0],
                                     self.switches[eachLod][1])

    def addLOD(self, lodName, inDist=0, outDist=0):
        """addLOD(self, string)
        Add a named node under the LODNode to parent all geometry
        of a specific LOD under.
        """
        self.__LODNode.attachNewNode(str(lodName))
        # save the switch distance info
        self.switches[lodName] = [inDist, outDist]
        # add the switch distance info
        self.__LODNode.node().addSwitch(inDist, outDist)

    def setLOD(self, lodName, inDist=0, outDist=0):
        """setLOD(self, string)
        Set the switch distance for given LOD
        """
        # save the switch distance info
        self.switches[lodName] = [inDist, outDist]
        # add the switch distance info
        sortedKeys = self.switches.keys()
        sortedKeys.sort()
        index = sortedKeys.index(lodName)
        self.__LODNode.node().setSwitch(index, inDist, outDist)

    def getLOD(self, lodName):
        """getLOD(self, string)
        Get the named node under the LOD to which we parent all LOD
        specific geometry to. Returns 'None' if not found
        """
        lod = self.__LODNode.find("**/" + str(lodName))
        if lod.isEmpty():
            return None
        else:
            return lod

    def hasLOD(self):
        """
        Return 1 if the actor has LODs, 0 otherwise
        """
        return self.__hasLOD

    def update(self, lod=0):
        lodnames = self.getLODNames()
        if (lod < len(lodnames)):
            partBundles = self.__partBundleDict[lodnames[lod]].values()
            for partBundle in partBundles:
                # print "updating: %s" % (partBundle.node())
                partBundle.node().updateToNow()
        else:
            self.notify.warning('update() - no lod: %d' % lod)

    def getFrameRate(self, animName=None, partName=None):
        """getFrameRate(self, string, string=None)
        Return actual frame rate of given anim name and given part.
        If no anim specified, use the currently playing anim.
        If no part specified, return anim durations of first part.
        NOTE: returns info only for an arbitrary LOD
        """
        lodName = self.__animControlDict.keys()[0]
        controls = self.getAnimControls(animName, partName)
        if len(controls) == 0:
            return None

        return controls[0].getFrameRate()

    def getBaseFrameRate(self, animName=None, partName=None):
        """getBaseFrameRate(self, string, string=None)
        Return frame rate of given anim name and given part, unmodified
        by any play rate in effect.
        """
        lodName = self.__animControlDict.keys()[0]
        controls = self.getAnimControls(animName, partName)
        if len(controls) == 0:
            return None

        return controls[0].getAnim().getBaseFrameRate()

    def getPlayRate(self, animName=None, partName=None):
        """
        Return the play rate of given anim for a given part.
        If no part is given, assume first part in dictionary.
        If no anim is given, find the current anim for the part.
        NOTE: Returns info only for an arbitrary LOD
        """
        # use the first lod
        lodName = self.__animControlDict.keys()[0]
        controls = self.getAnimControls(animName, partName)
        if len(controls) == 0:
            return None

        return controls[0].getPlayRate()

    def setPlayRate(self, rate, animName, partName=None):
        """setPlayRate(self, float, string, string=None)
        Set the play rate of given anim for a given part.
        If no part is given, set for all parts in dictionary.

        It used to be legal to let the animName default to the
        currently-playing anim, but this was confusing and could lead
        to the wrong anim's play rate getting set.  Better to insist
        on this parameter.
        NOTE: sets play rate on all LODs"""
        for control in self.getAnimControls(animName, partName):
            control.setPlayRate(rate)

    def getDuration(self, animName=None, partName=None,
                    fromFrame=None, toFrame=None):
        """
        Return duration of given anim name and given part.
        If no anim specified, use the currently playing anim.
        If no part specified, return anim duration of first part.
        NOTE: returns info for arbitrary LOD
        """
        lodName = self.__animControlDict.keys()[0]
        controls = self.getAnimControls(animName, partName)
        if len(controls) == 0:
            return None

        animControl = controls[0]
        if fromFrame is None:
            fromFrame = 0
        if toFrame is None:
            toFrame = animControl.getNumFrames()-1
        return ((toFrame+1)-fromFrame) / animControl.getFrameRate()

    def getNumFrames(self, animName=None, partName=None):
        lodName = self.__animControlDict.keys()[0]
        controls = self.getAnimControls(animName, partName)
        if len(controls) == 0:
            return None
        return controls[0].getNumFrames()

    def getFrameTime(self, anim, frame):
        numFrames = self.getNumFrames(anim)
        animTime = self.getDuration(anim)
        frameTime = animTime * float(frame) / numFrames
        return frameTime

    def getCurrentAnim(self, partName=None):
        """
        Return the anim currently playing on the actor. If part not
        specified return current anim of an arbitrary part in dictionary.
        NOTE: only returns info for an arbitrary LOD
        """
        lodName, animControlDict = self.__animControlDict.items()[0]
        if partName == None:
            partName, animDict = animControlDict.items()[0]
        else:
            animDict = animControlDict.get(partName)
            if animDict == None:
                # part was not present
                Actor.notify.warning("couldn't find part: %s" % (partName))
                return None

        # loop through all anims for named part and find if any are playing
        for animName, anim in animDict.items():
            if isinstance(anim[1], AnimControl) and anim[1].isPlaying():
                return animName

        # we must have found none, or gotten an error
        return None

    def getCurrentFrame(self, animName=None, partName=None):
        """
        Return the current frame number of the anim current playing on
        the actor. If part not specified return current anim of first
        part in dictionary.
        NOTE: only returns info for an arbitrary LOD
        """
        lodName, animControlDict = self.__animControlDict.items()[0]
        if partName == None:
            partName, animDict = animControlDict.items()[0]
        else:
            animDict = animControlDict.get(partName)
            if animDict == None:
                # part was not present
                Actor.notify.warning("couldn't find part: %s" % (partName))
                return None

        # loop through all anims for named part and find if any are playing
        for animName, anim in animDict.items():
            if isinstance(anim[1], AnimControl) and anim[1].isPlaying():
                return anim[1].getFrame()

        # we must have found none, or gotten an error
        return None


    # arranging

    def getPart(self, partName, lodName="lodRoot"):
        """
        Find the named part in the optional named lod and return it, or
        return None if not present
        """
        partBundleDict = self.__partBundleDict.get(lodName)
        if not partBundleDict:
            Actor.notify.warning("no lod named: %s" % (lodName))
            return None
        truePartName = self.__subpartDict.get(partName, [partName])[0]
        return partBundleDict.get(truePartName)

    def removePart(self, partName, lodName="lodRoot"):
        """
        Remove the geometry and animations of the named part of the
        optional named lod if present.
        NOTE: this will remove child geometry also!
        """
        # find the corresponding part bundle dict
        partBundleDict = self.__partBundleDict.get(lodName)
        if not partBundleDict:
            Actor.notify.warning("no lod named: %s" % (lodName))
            return

        # remove the part
        if (partBundleDict.has_key(partName)):
            partBundleDict[partName].removeNode()
            del(partBundleDict[partName])

        # find the corresponding anim control dict
        partDict = self.__animControlDict.get(lodName)
        if not partDict:
            Actor.notify.warning("no lod named: %s" % (lodName))
            return

        # remove the animations
        if (partDict.has_key(partName)):
            del(partDict[partName])

    def hidePart(self, partName, lodName="lodRoot"):
        """
        Make the given part of the optionally given lod not render,
        even though still in the tree.
        NOTE: this will affect child geometry
        """
        partBundleDict = self.__partBundleDict.get(lodName)
        if not partBundleDict:
            Actor.notify.warning("no lod named: %s" % (lodName))
            return
        part = partBundleDict.get(partName)
        if part:
            part.hide()
        else:
            Actor.notify.warning("no part named %s!" % (partName))

    def showPart(self, partName, lodName="lodRoot"):
        """
        Make the given part render while in the tree.
        NOTE: this will affect child geometry
        """
        partBundleDict = self.__partBundleDict.get(lodName)
        if not partBundleDict:
            Actor.notify.warning("no lod named: %s" % (lodName))
            return
        part = partBundleDict.get(partName)
        if part:
            part.show()
        else:
            Actor.notify.warning("no part named %s!" % (partName))

    def showAllParts(self, partName, lodName="lodRoot"):
        """
        Make the given part and all its children render while in the tree.
        NOTE: this will affect child geometry
        """
        partBundleDict = self.__partBundleDict.get(lodName)
        if not partBundleDict:
            Actor.notify.warning("no lod named: %s" % (lodName))
            return
        part = partBundleDict.get(partName)
        if part:
            part.show()
            part.getChildren().show()
        else:
            Actor.notify.warning("no part named %s!" % (partName))

    def exposeJoint(self, node, partName, jointName, lodName="lodRoot",
                    localTransform = 0):
        """exposeJoint(self, NodePath, string, string, key="lodRoot")
        Starts the joint animating the indicated node.  As the joint
        animates, it will transform the node by the corresponding
        amount.  This will replace whatever matrix is on the node each
        frame.  The default is to expose the net transform from the root,
        but if localTransform is true, only the node's local transform
        from its parent is exposed."""
        partBundleDict = self.__partBundleDict.get(lodName)
        if not partBundleDict:
            Actor.notify.warning("no lod named: %s" % (lodName))
            return None

        truePartName = self.__subpartDict.get(partName, [partName])[0]
        part = partBundleDict.get(truePartName)
        if part:
            bundle = part.node().getBundle()
        else:
            Actor.notify.warning("no part named %s!" % (partName))
            return None

        # Get a handle to the joint.
        joint = bundle.findChild(jointName)

        if node == None:
            node = self.attachNewNode(jointName)

        if (joint):
            if localTransform:
                joint.addLocalTransform(node.node())
            else:
                joint.addNetTransform(node.node())
        else:
            Actor.notify.warning("no joint named %s!" % (jointName))

        return node

    def stopJoint(self, partName, jointName, lodName="lodRoot"):
        """stopJoint(self, string, string, key="lodRoot")
        Stops the joint from animating external nodes.  If the joint
        is animating a transform on a node, this will permanently stop
        it.  However, this does not affect vertex animations."""
        partBundleDict = self.__partBundleDict.get(lodName)
        if not partBundleDict:
            Actor.notify.warning("no lod named: %s" % (lodName))
            return None

        truePartName = self.__subpartDict.get(partName, [partName])[0]
        part = partBundleDict.get(truePartName)
        if part:
            bundle = part.node().getBundle()
        else:
            Actor.notify.warning("no part named %s!" % (partName))
            return None

        # Get a handle to the joint.
        joint = bundle.findChild(jointName)

        if (joint):
            joint.clearNetTransforms()
            joint.clearLocalTransforms()
        else:
            Actor.notify.warning("no joint named %s!" % (jointName))

    def controlJoint(self, node, partName, jointName, lodName="lodRoot"):
        """controlJoint(self, NodePath, string, string, key="lodRoot")

        The converse of exposeJoint: this associates the joint with
        the indicated node, so that the joint transform will be copied
        from the node to the joint each frame.  This can be used for
        programmer animation of a particular joint at runtime.

        This must be called before any animations are played.  Once an
        animation has been loaded and bound to the character, it will
        be too late to add a new control during that animation.
        """
        partBundleDict = self.__partBundleDict.get(lodName)
        if not partBundleDict:
            Actor.notify.warning("no lod named: %s" % (lodName))
            return None

        truePartName = self.__subpartDict.get(partName, [partName])[0]
        part = partBundleDict.get(truePartName)
        if part:
            bundle = part.node().getBundle()
        else:
            Actor.notify.warning("no part named %s!" % (partName))
            return None

        joint = bundle.findChild(jointName)
        if joint == None:
            Actor.notify.warning("no joint named %s!" % (jointName))
            return None

        if node == None:
            node = self.attachNewNode(jointName)
            if joint.getType().isDerivedFrom(MovingPartMatrix.getClassType()):
                node.setMat(joint.getInitialValue())

        # Store a dictionary of jointName: node to list the controls
        # requested for joints.  The controls will actually be applied
        # later, when we load up the animations in bindAnim().
        if self.__controlJoints.has_key(bundle.this):
            self.__controlJoints[bundle.this][jointName] = node
        else:
            self.__controlJoints[bundle.this] = { jointName: node }

        return node

    def instance(self, path, partName, jointName, lodName="lodRoot"):
        """instance(self, NodePath, string, string, key="lodRoot")
        Instance a nodePath to an actor part at a joint called jointName"""
        partBundleDict = self.__partBundleDict.get(lodName)
        if partBundleDict:
            truePartName = self.__subpartDict.get(partName, [partName])[0]
            part = partBundleDict.get(truePartName)
            if part:
                joint = part.find("**/" + jointName)
                if (joint.isEmpty()):
                    Actor.notify.warning("%s not found!" % (jointName))
                else:
                    return path.instanceTo(joint)
            else:
                Actor.notify.warning("no part named %s!" % (partName))
        else:
            Actor.notify.warning("no lod named %s!" % (lodName))

    def attach(self, partName, anotherPartName, jointName, lodName="lodRoot"):
        """attach(self, string, string, string, key="lodRoot")
        Attach one actor part to another at a joint called jointName"""
        partBundleDict = self.__partBundleDict.get(lodName)
        if partBundleDict:
            truePartName = self.__subpartDict.get(partName, [partName])[0]
            part = partBundleDict.get(truePartName)
            if part:
                anotherPart = partBundleDict.get(anotherPartName)
                if anotherPart:
                    joint = anotherPart.find("**/" + jointName)
                    if (joint.isEmpty()):
                        Actor.notify.warning("%s not found!" % (jointName))
                    else:
                        part.reparentTo(joint)
                else:
                    Actor.notify.warning("no part named %s!" % (anotherPartName))
            else:
                Actor.notify.warning("no part named %s!" % (partName))
        else:
            Actor.notify.warning("no lod named %s!" % (lodName))


    def drawInFront(self, frontPartName, backPartName, mode,
                    root=None, lodName=None):
        """drawInFront(self, string, int, string=None, key=None)

        Arrange geometry so the frontPart(s) are drawn in front of
        backPart.

        If mode == -1, the geometry is simply arranged to be drawn in
        the correct order, assuming it is already under a
        direct-render scene graph (like the DirectGui system).  That
        is, frontPart is reparented to backPart, and backPart is
        reordered to appear first among its siblings.

        If mode == -2, the geometry is arranged to be drawn in the
        correct order, and depth test/write is turned off for
        frontPart.

        If mode == -3, frontPart is drawn as a decal onto backPart.
        This assumes that frontPart is mostly coplanar with and does
        not extend beyond backPart, and that backPart is mostly flat
        (not self-occluding).

        If mode > 0, the frontPart geometry is placed in the 'fixed'
        bin, with the indicated drawing order.  This will cause it to
        be drawn after almost all other geometry.  In this case, the
        backPartName is actually unused.

        Takes an optional argument root as the start of the search for the
        given parts. Also takes optional lod name to refine search for the
        named parts. If root and lod are defined, we search for the given
        root under the given lod.
        """
        # check to see if we are working within an lod
        if lodName != None:
            # find the named lod node
            lodRoot = self.find("**/" + str(lodName))
            if root == None:
                # no need to look further
                root = lodRoot
            else:
                # look for root under lod
                root = lodRoot.find("**/" + root)
        else:
            # start search from self if no root and no lod given
            if root == None:
                root = self

        frontParts = root.findAllMatches("**/" + frontPartName)

        if mode > 0:
            # Use the 'fixed' bin instead of reordering the scene
            # graph.
            numFrontParts = frontParts.getNumPaths()
            for partNum in range(0, numFrontParts):
                frontParts[partNum].setBin('fixed', mode)
            return

        if mode == -2:
            # Turn off depth test/write on the frontParts.
            numFrontParts = frontParts.getNumPaths()
            for partNum in range(0, numFrontParts):
                frontParts[partNum].setDepthWrite(0)
                frontParts[partNum].setDepthTest(0)

        # Find the back part.
        backPart = root.find("**/" + backPartName)
        if (backPart.isEmpty()):
            Actor.notify.warning("no part named %s!" % (backPartName))
            return

        if mode == -3:
            # Draw as a decal.
            backPart.node().setEffect(DecalEffect.make())
        else:
            # Reorder the backPart to be the first of its siblings.
            backPart.reparentTo(backPart.getParent(), -1)

        #reparent all the front parts to the back part
        frontParts.reparentTo(backPart)


    def fixBounds(self, part=None):
        """fixBounds(self, nodePath=None)
        Force recomputation of bounding spheres for all geoms
        in a given part. If no part specified, fix all geoms
        in this actor
        """
        # if no part name specified fix all parts
        if (part==None):
            part = self

        # update all characters first
        charNodes = part.findAllMatches("**/+Character")
        numCharNodes = charNodes.getNumPaths()
        for charNum in range(0, numCharNodes):
            (charNodes.getPath(charNum)).node().update()

        # for each geomNode, iterate through all geoms and force update
        # of bounding spheres by marking current bounds as stale
        geomNodes = part.findAllMatches("**/+GeomNode")
        numGeomNodes = geomNodes.getNumPaths()
        for nodeNum in range(0, numGeomNodes):
            thisGeomNode = geomNodes.getPath(nodeNum)
            numGeoms = thisGeomNode.node().getNumGeoms()
            for geomNum in range(0, numGeoms):
                thisGeom = thisGeomNode.node().getGeom(geomNum)
                thisGeom.markBoundStale()
                Actor.notify.debug("fixing bounds for node %s, geom %s" % \
                                  (nodeNum, geomNum))
            thisGeomNode.node().markBoundStale()

    def showAllBounds(self):
        """
        Show the bounds of all actor geoms
        """
        geomNodes = self.__geomNode.findAllMatches("**/+GeomNode")
        numGeomNodes = geomNodes.getNumPaths()

        for nodeNum in range(0, numGeomNodes):
            geomNodes.getPath(nodeNum).showBounds()

    def hideAllBounds(self):
        """
        Hide the bounds of all actor geoms
        """
        geomNodes = self.__geomNode.findAllMatches("**/+GeomNode")
        numGeomNodes = geomNodes.getNumPaths()

        for nodeNum in range(0, numGeomNodes):
            geomNodes.getPath(nodeNum).hideBounds()


    # actions
    def animPanel(self):
        from direct.showbase import TkGlobal
        from direct.tkpanels import AnimPanel
        return AnimPanel.AnimPanel(self)

    def stop(self, animName=None, partName=None):
        """stop(self, string=None, string=None)
        Stop named animation on the given part of the actor.
        If no name specified then stop all animations on the actor.
        NOTE: stops all LODs"""
        for control in self.getAnimControls(animName, partName):
            control.stop()

    def play(self, animName, partName=None, fromFrame=None, toFrame=None):
        """play(self, string, string=None)
        Play the given animation on the given part of the actor.
        If no part is specified, try to play on all parts. NOTE:
        plays over ALL LODs"""
        if fromFrame == None:
            for control in self.getAnimControls(animName, partName):
                control.play()
        else:
            for control in self.getAnimControls(animName, partName):
                if toFrame == None:
                    control.play(fromFrame, control.getNumFrames() - 1)
                else:
                    control.play(fromFrame, toFrame)

    def loop(self, animName, restart=1, partName=None,
             fromFrame=None, toFrame=None):
        """loop(self, string, int=1, string=None)
        Loop the given animation on the given part of the actor,
        restarting at zero frame if requested. If no part name
        is given then try to loop on all parts. NOTE: loops on
        all LOD's
        """
        if fromFrame == None:
            for control in self.getAnimControls(animName, partName):
                control.loop(restart)
        else:
            for control in self.getAnimControls(animName, partName):
                if toFrame == None:
                    control.loop(restart, fromFrame, control.getNumFrames() - 1)
                else:
                    control.loop(restart, fromFrame, toFrame)

    def pingpong(self, animName, restart=1, partName=None,
                 fromFrame=None, toFrame=None):
        """pingpong(self, string, int=1, string=None)
        Loop the given animation on the given part of the actor,
        restarting at zero frame if requested. If no part name
        is given then try to loop on all parts. NOTE: loops on
        all LOD's"""
        if fromFrame == None:
            fromFrame = 0

        for control in self.getAnimControls(animName, partName):
            if toFrame == None:
                control.pingpong(restart, fromFrame, control.getNumFrames() - 1)
            else:
                control.pingpong(restart, fromFrame, toFrame)

    def pose(self, animName, frame, partName=None, lodName=None):
        """pose(self, string, int, string=None)
        Pose the actor in position found at given frame in the specified
        animation for the specified part. If no part is specified attempt
        to apply pose to all parts."""
        for control in self.getAnimControls(animName, partName, lodName):
            control.pose(frame)

    def enableBlend(self, blendType = PartBundle.BTNormalizedLinear, partName = None):
        """
        Enables blending of multiple animations simultaneously.
        After this is called, you may call play(), loop(), or pose()
        on multiple animations and have all of them contribute to the
        final pose each frame.

        With blending in effect, starting a particular animation with
        play(), loop(), or pose() does not implicitly make the
        animation visible; you must also call setControlEffect() for
        each animation you wish to use to indicate how much each
        animation contributes to the final pose.
        """
        for lodName, bundleDict in self.__partBundleDict.items():
            if partName == None:
                for partBundle in bundleDict.values():
                    partBundle.node().getBundle().setBlendType(blendType)
            else:
                truePartName = self.__subpartDict.get(partName, [partName])[0]
                partBundle = bundleDict.get(truePartName)
                if partBundle != None:
                    partBundle.node().getBundle().setBlendType(blendType)
                else:
                    Actor.notify.warning("Couldn't find part: %s" % (partName))

    def disableBlend(self, partName = None):
        """
        Restores normal one-animation-at-a-time operation after a
        previous call to enableBlend().
        """
        self.enableBlend(PartBundle.BTSingle, partName)

    def setControlEffect(self, animName, effect,
                         partName = None, lodName = None):
        """
        Sets the amount by which the named animation contributes to
        the overall pose.  This controls blending of multiple
        animations; it only makes sense to call this after a previous
        call to enableBlend().
        """
        for control in self.getAnimControls(animName, partName, lodName):
            control.getPart().setControlEffect(control, effect)

    def getAnimControl(self, animName, partName, lodName="lodRoot"):
        """getAnimControl(self, string, string, string="lodRoot")
        Search the animControl dictionary indicated by lodName for
        a given anim and part. Return the animControl if present,
        or None otherwise
        """
        partDict = self.__animControlDict.get(lodName)
        # if this assertion fails, named lod was not present
        assert partDict != None

        animDict = partDict.get(partName)
        if animDict == None:
            # part was not present
            Actor.notify.warning("couldn't find part: %s" % (partName))
        else:
            anim = animDict.get(animName)
            if anim == None:
                # anim was not present
                Actor.notify.debug("couldn't find anim: %s" % (animName))
            else:
                # bind the animation first if we need to
                if not isinstance(anim[1], AnimControl):
                    self.__bindAnimToPart(animName, partName, lodName)
                return anim[1]

        return None

    def getAnimControls(self, animName=None, partName=None, lodName=None):
        """getAnimControls(self, string, string=None, string=None)

        Returns a list of the AnimControls that represent the given
        animation for the given part and the given lod.  If animName
        is omitted, the currently-playing animation (or all
        currently-playing animations) is returned.  If partName is
        omitted, all parts are returned.  If lodName is omitted, all
        LOD's are returned.
        """
        controls = []

        # build list of lodNames and corresponding animControlDicts
        # requested.
        if lodName == None:
            # Get all LOD's
            animControlDictItems = self.__animControlDict.items()
        else:
            partDict = self.__animControlDict.get(lodName)
            if partDict == None:
                Actor.notify.warning("couldn't find lod: %s" % (lodName))
                animControlDictItems = []
            else:
                animControlDictItems = [(lodName, partDict)]

        for lodName, partDict in animControlDictItems:
            # Now, build the list of partNames and the corresponding
            # animDicts.
            if partName == None:
                # Get all main parts, but not sub-parts.
                animDictItems = []
                for thisPart, animDict in partDict.items():
                    if not self.__subpartDict.has_key(thisPart):
                        animDictItems.append((thisPart, animDict))

            else:
                # Get exactly the named part or parts.
                if isinstance(partName, types.StringTypes):
                    partNameList = [partName]
                else:
                    partNameList = partName

                animDictItems = []
                for partName in partNameList:
                    animDict = partDict.get(partName)
                    if animDict == None:
                        # Maybe it's a subpart that hasn't been bound yet.
                        subpartDef = self.__subpartDict.get(partName)
                        if subpartDef:
                            animDict = {}
                            partDict[partName] = animDict

                    if animDict == None:
                        # part was not present
                        Actor.notify.warning("couldn't find part: %s" % (partName))
                    else:
                        animDictItems.append((partName, animDict))

            if animName == None:
                # get all playing animations
                for thisPart, animDict in animDictItems:
                    for anim in animDict.values():
                        if isinstance(anim[1], AnimControl) and anim[1].isPlaying():
                            controls.append(anim[1])
            else:
                # get the named animation only.
                for thisPart, animDict in animDictItems:
                    anim = animDict.get(animName)
                    if anim == None:
                        # Maybe it's a subpart that hasn't been bound yet.
                        subpartDef = self.__subpartDict.get(partName)
                        if subpartDef:
                            truePartName = subpartDef[0]
                            anim = partDict[truePartName].get(animName)
                            if anim:
                                anim = [anim[0], None]
                                animDict[animName] = anim

                    if anim == None:
                        # anim was not present
                        Actor.notify.debug("couldn't find anim: %s" % (animName))
                    else:
                        # bind the animation first if we need to
                        animControl = anim[1]
                        if animControl == None:
                            animControl = self.__bindAnimToPart(animName, thisPart, lodName)
                        if animControl:
                            controls.append(animControl)

        return controls

    def loadModel(self, modelPath, partName="modelRoot", lodName="lodRoot", copy = 1):
        """loadModel(self, string, string="modelRoot", string="lodRoot",
        bool = 0)
        Actor model loader. Takes a model name (ie file path), a part
        name(defaults to "modelRoot") and an lod name(defaults to "lodRoot").
        If copy is set to 0, do a loadModel instead of a loadModelCopy.
        """
        assert partName not in self.__subpartDict

        Actor.notify.debug("in loadModel: %s, part: %s, lod: %s, copy: %s" % \
            (modelPath, partName, lodName, copy))

        if isinstance(modelPath, NodePath):
            # If we got a NodePath instead of a string, use *that* as
            # the model directly.
            if (copy):
                model = modelPath.copyTo(hidden)
            else:
                model = modelPath
        else:
            # otherwise, we got the name of the model to load.
            if (copy):
                # We can't pass loaderOptions to loadModelCopy.
                model = loader.loadModelCopy(modelPath)
            else:
                # But if we're loading our own copy of the model, we
                # can pass loaderOptions to specify that we want to
                # get the skeleton model.  This only matters to model
                # files (like .mb) for which we can choose to extract
                # either the skeleton or animation, or neither.
                model = loader.loadModel(modelPath,
                                         loaderOptions = self.modelLoaderOptions)

        if (model == None):
            raise StandardError, "Could not load Actor model %s" % (modelPath)

        bundle = model.find("**/+PartBundleNode")
        if (bundle.isEmpty()):
            Actor.notify.warning("%s is not a character!" % (modelPath))
            model.reparentTo(self.__geomNode)
        else:
            # Maybe the model file also included some animations.  If
            # so, try to bind them immediately and put them into the
            # animControlDict.
            acc = AnimControlCollection()
            autoBind(model.node(), acc, ~0)
            numAnims = acc.getNumAnims()

            # Now extract out the PartBundleNode and integrate it with
            # the Actor.
            self.prepareBundle(bundle, partName, lodName)

            if numAnims != 0:
                # If the model had some animations, store them in the
                # dict so they can be played.
                Actor.notify.info("model contains %s animations." % (numAnims))

                # make sure this lod is in anim control dict
                self.__animControlDict.setdefault(lodName, {})
                self.__animControlDict[lodName].setdefault(partName, {})

                for i in range(numAnims):
                    animControl = acc.getAnim(i)
                    animName = acc.getAnimName(i)

                    # Now we've already bound the animation, but we
                    # have no associated filename.  So store the
                    # animControl, but put None in for the filename.
                    self.__animControlDict[lodName][partName][animName] = [None, animControl]

            model.removeNode()

    def prepareBundle(self, bundle, partName="modelRoot", lodName="lodRoot"):
        assert partName not in self.__subpartDict

        # Rename the node at the top of the hierarchy, if we
        # haven't already, to make it easier to identify this
        # actor in the scene graph.
        if not self.gotName:
            self.node().setName(bundle.node().getName())
            self.gotName = 1

        # we rename this node to make Actor copying easier
        bundle.node().setName(Actor.partPrefix + partName)

        if (self.__partBundleDict.has_key(lodName) == 0):
            # make a dictionary to store these parts in
            needsDict = 1
            bundleDict = {}
        else:
            needsDict = 0

        if (lodName!="lodRoot"):
            # instance to appropriate node under LOD switch
            #bundle = bundle.instanceTo(
            #    self.__LODNode.find("**/" + str(lodName)))
            bundle.reparentTo(self.__LODNode.find("**/" + str(lodName)))
        else:
            #bundle = bundle.instanceTo(self.__geomNode)
            bundle.reparentTo(self.__geomNode)

        if (needsDict):
            bundleDict[partName] = bundle
            self.__partBundleDict[lodName] = bundleDict
            self.__updateSortedLODNames()
        else:
            self.__partBundleDict[lodName][partName] = bundle

    def makeSubpart(self, partName, includeJoints, excludeJoints = [],
                    parent="modelRoot"):

        """Defines a new "part" of the Actor that corresponds to the
        same geometry as the named parent part, but animates only a
        certain subset of the joints.  This can be used for
        partial-body animations, for instance to animate a hand waving
        while the rest of the body continues to play its walking
        animation.

        includeJoints is a list of joint names that are to be animated
        by the subpart.  Each name can include globbing characters
        like '?' or '*', which will match one or any number of
        characters, respectively.  Including a joint by naming it in
        includeJoints implicitly includes all of the descendents of
        that joint as well, except for excludeJoints, below.

        excludeJoints is a list of joint names that are *not* to be
        animated by the subpart.  As in includeJoints, each name can
        include globbing characters.  If a joint is named by
        excludeJoints, it will not be included (and neither will any
        of its descendents), even if a parent joint was named by
        includeJoints.

        parent is the actual partName that this subpart is based
        on."""

        assert partName not in self.__subpartDict

        truePartName = partName
        prevSubset = PartSubset()
        subpartDef = self.__subpartDict.get(partName)
        if subpartDef:
            truePartName, subset = subpartDef

        subset = PartSubset(prevSubset)
        for name in includeJoints:
            subset.addIncludeJoint(GlobPattern(name))
        for name in excludeJoints:
            subset.addExcludeJoint(GlobPattern(name))

        self.__subpartDict[partName] = (parent, subset)

    def loadAnims(self, anims, partName="modelRoot", lodName="lodRoot"):
        """loadAnims(self, string:string{}, string='modelRoot',
        string='lodRoot')
        Actor anim loader. Takes an optional partName (defaults to
        'modelRoot' for non-multipart actors) and lodName (defaults
        to 'lodRoot' for non-LOD actors) and dict of corresponding
        anims in the form animName:animPath{}
        """
        Actor.notify.debug("in loadAnims: %s, part: %s, lod: %s" %
                           (anims, partName, lodName))

        for animName in anims.keys():
            # make sure this lod is in anim control dict
            self.__animControlDict.setdefault(lodName, {})
            self.__animControlDict[lodName].setdefault(partName, {})
            # store the file path and None in place of the animControl.
            # we will bind it only when played
            self.__animControlDict[lodName][partName][animName] = [anims[animName], None]


    def unloadAnims(self, anims, partName="modelRoot", lodName="lodRoot"):
        """unloadAnims(self, string:string{}, string='modelRoot',
        string='lodRoot')
        Actor anim unloader. Takes an optional partName (defaults to
        'modelRoot' for non-multipart actors) and lodName (defaults
        to 'lodRoot' for non-LOD actors) and dict of corresponding
        anims in the form animName:animPath{}. Deletes the anim control
        for the given animation and parts/lods.
        """
        Actor.notify.debug("in unloadAnims: %s, part: %s, lod: %s" %
                           (anims, partName, lodName))

        if (lodName == None):
            lodNames = self.__animControlDict.keys()
        else:
            lodNames = [lodName]

        if (partName == None):
            if len(lodNames) > 0:
                partNames = self.__animControlDict[lodNames[0]].keys()
            else:
                partNames = []
        else:
            partNames = [partName]

        if (anims==None):
            if len(lodNames) > 0 and len(partNames) > 0:
                anims = self.__animControlDict[lodNames[0]][partNames[0]].keys()
            else:
                anims = []

        for lodName in lodNames:
            for partName in partNames:
                for animName in anims:
                    # delete the anim control
                    animControlPair = self.__animControlDict[lodName][partName][animName]
                    if animControlPair[1] != None:
                        # Try to clear any control effects before we let
                        # our handle on them go. This is especially
                        # important if the anim control was blending
                        # animations.
                        animControlPair[1].getPart().clearControlEffects()
                        del(animControlPair[1])
                        animControlPair.append(None)

    def bindAnim(self, animName, partName="modelRoot", lodName="lodRoot"):
        """bindAnim(self, string, string='modelRoot', string='lodRoot')
        Bind the named animation to the named part and lod
        """
        if lodName == None:
            lodNames = self.__animControlDict.keys()
        else:
            lodNames = [lodName]

        # loop over all lods
        for thisLod in lodNames:
            if partName == None:
                partNames = self.__partBundleDict[thisLod].keys()
            else:
                partNames = [partName]
            # loop over all parts
            for thisPart in partNames:
                ac = self.__bindAnimToPart(animName, thisPart, thisLod)


    def __bindAnimToPart(self, animName, partName, lodName):
        """
        for internal use only!
        """
        # make sure this anim is in the dict

        subpartDef = self.__subpartDict.get(partName)
        truePartName = partName
        subset = PartSubset()
        if subpartDef:
            truePartName, subset = subpartDef


        partDict = self.__animControlDict[lodName]
        animDict = partDict.get(partName)
        if animDict == None:
            # It must be a subpart that hasn't been bound yet.
            animDict = {}
            partDict[partName] = animDict

        anim = animDict.get(animName)
        if anim == None:
            # It must be a subpart that hasn't been bound yet.
            if subpartDef != None:
                anim = partDict[truePartName].get(animName)
                anim = [anim[0], None]
                animDict[animName] = anim

        if anim == None:
            Actor.notify.error("actor has no animation %s", animName)

        # only bind if not already bound!
        if anim[1]:
            return anim[1]

        # fetch a copy from the modelPool, or if we weren't careful
        # enough to preload, fetch from disk
        animPath = anim[0]
        if self.__autoCopy:
            animNode = loader.loadModelOnce(animPath)
        else:
            animNode = loader.loadModel(animPath,
                                        loaderOptions = self.animLoaderOptions)
        if animNode == None:
            return None
        animBundle = (animNode.find("**/+AnimBundleNode").node()).getBundle()

        bundle = self.__partBundleDict[lodName][truePartName].node().getBundle()

        # Are there any controls requested for joints in this bundle?
        # If so, apply them.
        controlDict = self.__controlJoints.get(bundle.this, None)
        if controlDict:
            for jointName, node in controlDict.items():
                if node:
                    joint = animBundle.makeChildDynamic(jointName)
                    if joint:
                        joint.setValueNode(node.node())
                    else:
                        Actor.notify.error("controlled joint %s is not present" % jointName)

        # bind anim
        animControl = bundle.bindAnim(animBundle, -1, subset)

        if (animControl == None):
            Actor.notify.error("Null AnimControl: %s" % (animName))
        else:
            # store the animControl
            anim[1] = animControl
            Actor.notify.debug("binding anim: %s to part: %s, lod: %s" %
                               (animName, partName, lodName))
        return animControl

    def __copyPartBundles(self, other):
        """__copyPartBundles(self, Actor)
        Copy the part bundle dictionary from another actor as this
        instance's own. NOTE: this method does not actually copy geometry
        """
        for lodName in other.__partBundleDict.keys():
            self.__partBundleDict[lodName] = {}
            self.__updateSortedLODNames()
            # find the lod Asad
            if lodName == 'lodRoot':
                partLod = self
            else:
                partLod = self.find("**/" + lodName)
            if partLod.isEmpty():
                Actor.notify.warning("no lod named: %s" % (lodName))
                return None
            for partName in other.__partBundleDict[lodName].keys():
                # find the part in our tree
                #partBundle = self.find("**/" + Actor.partPrefix + partName)
                # Asad: changed above line to below
                partBundle = partLod.find("**/" + Actor.partPrefix + partName)
                if (partBundle != None):
                    # store the part bundle
                    self.__partBundleDict[lodName][partName] = partBundle
                else:
                    Actor.notify.error("lod: %s has no matching part: %s" %
                                       (lodName, partName))


    def __copyAnimControls(self, other):
        """__copyAnimControls(self, Actor)
        Get the anims from the anim control's in the anim control
        dictionary of another actor. Bind these anim's to the part
        bundles in our part bundle dict that have matching names, and
        store the resulting anim controls in our own part bundle dict"""
        for lodName in other.__animControlDict.keys():
            self.__animControlDict[lodName] = {}
            for partName in other.__animControlDict[lodName].keys():
                self.__animControlDict[lodName][partName] = {}
                for animName in other.__animControlDict[lodName][partName].keys():
                    # else just copy what's there
                    self.__animControlDict[lodName][partName][animName] = \
                        [other.__animControlDict[lodName][partName][animName][0], None]

    def actorInterval(self, *args, **kw):
        from direct.interval import ActorInterval
        return ActorInterval.ActorInterval(self, *args, **kw)

    def printAnimBlends(self, animName=None, partName=None, lodName=None):
        out = ''
        first = True
        if animName is None:
            animNames = self.getAnimNames()
        else:
            animNames = [animName]
        for animName in animNames:
            if animName is 'nothing':
                continue
            thisAnim = '%s: ' % animName
            totalEffect = 0.
            controls = self.getAnimControls(animName, partName, lodName)
            for control in controls:
                part = control.getPart()
                name = part.getName()
                effect = part.getControlEffect(control)
                if effect > 0.:
                    totalEffect += effect
                    thisAnim += ('%s:%.3f, ' % (name, effect))
            # don't print anything if this animation is not being played
            if totalEffect > 0.:
                if not first:
                    out += '\n'
                first = False
                out += thisAnim
        print out

    def osdAnimBlends(self, animName=None, partName=None, lodName=None):
        if not onScreenDebug.enabled:
            return
        # puts anim blending info into the on-screen debug panel
        if animName is None:
            animNames = self.getAnimNames()
        else:
            animNames = [animName]
        for animName in animNames:
            if animName is 'nothing':
                continue
            thisAnim = ''
            totalEffect = 0.
            controls = self.getAnimControls(animName, partName, lodName)
            for control in controls:
                part = control.getPart()
                name = part.getName()
                effect = part.getControlEffect(control)
                if effect > 0.:
                    totalEffect += effect
                    thisAnim += ('%s:%.3f, ' % (name, effect))
            thisAnim += "\n"
            for control in controls:
                part = control.getPart()
                name = part.getName()
                rate = control.getPlayRate()
                thisAnim += ('%s:%.1f, ' % (name, rate))
            # don't display anything if this animation is not being played
            itemName = 'anim %s' % animName
            if totalEffect > 0.:
                onScreenDebug.add(itemName, thisAnim)
            else:
                if onScreenDebug.has(itemName):
                    onScreenDebug.remove(itemName)
