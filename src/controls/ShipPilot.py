"""
ShipPilot.py is for avatars pilotting ships (or more accurately, a ship
as the avatar).

A control such as this one provides:
    - creation of the collision nodes
    - handling the keyboard and mouse input for avatar movement
    - moving the avatar

it does not:
    - play sounds
    - play animations

although it does send messeges that allow a listener to play sounds or
animations based on control events.
"""

from direct.showbase.ShowBaseGlobal import *

from direct.directnotify.DirectNotifyGlobal import directNotify
from pandac.PandaModules import PhysicsManager
import math

from PhysicsWalker import PhysicsWalker

class ShipPilot(PhysicsWalker):

    notify = directNotify.newCategory("PhysicsWalker")
    wantDebugIndicator = base.config.GetBool(
        'want-avatar-physics-indicator', 0)

    useBowSternSpheres = 1
    useLifter = 0
    useHeightRay = 0

    # special methods
    def __init__(self, gravity = -32.1740, standableGround=0.707,
            hardLandingForce=16.0):
        assert self.debugPrint(
            "PhysicsWalker(gravity=%s, standableGround=%s)"%(
            gravity, standableGround))
        PhysicsWalker.__init__(
            self, gravity, standableGround, hardLandingForce)
        self.__gravity=gravity
        self.__standableGround=standableGround
        self.__hardLandingForce=hardLandingForce

        self.needToDeltaPos = 0
        self.physVelocityIndicator=None
        self.avatarControlForwardSpeed=0
        self.avatarControlJumpForce=0
        self.avatarControlReverseSpeed=0
        self.avatarControlRotateSpeed=0
        self.__oldAirborneHeight=None
        self.getAirborneHeight=None
        self.__oldContact=None
        self.__oldPosDelta=Vec3(0)
        self.__oldDt=0
        self.__speed=0.0
        self.__rotationSpeed=0.0
        self.__slideSpeed=0.0
        self.__vel=Vec3(0.0)
        self.collisionsActive = 0

        self.isAirborne = 0
        self.highMark = 0
        self.ship = None

    def setWalkSpeed(self, forward, jump, reverse, rotate):
        assert self.debugPrint("setWalkSpeed()")
        self.avatarControlForwardSpeed=forward
        self.avatarControlJumpForce=0.0
        self.avatarControlReverseSpeed=reverse
        self.avatarControlRotateSpeed=rotate

    def getSpeeds(self):
        #assert self.debugPrint("getSpeeds()")
        return (self.__speed, self.__rotationSpeed)

    def setAvatar(self, ship):
        if ship is None:
            self.takedownPhysics()
            self.ship = None
        else:
            #self.setupShip()
            self.setupPhysics(ship)
            self.ship = ship

            #*# Debug:
            if not hasattr(ship, "acceleration"):
                self.ship.acceleration = 60
                self.ship.maxSpeed = 14
                self.ship.reverseAcceleration = 10
                self.ship.maxReverseSpeed = 2
                self.ship.turnRate = 3
                self.ship.maxTurn = 30
                self.ship.anchorDrag = .9
                self.ship.hullDrag = .9

    def setupRay(self, floorBitmask, floorOffset):
        # This is a ray cast from your head down to detect floor polygons
        # A toon is about 4.0 feet high, so start it there
        self.cRay = CollisionRay(
            0.0, 0.0, CollisionHandlerRayStart, 0.0, 0.0, -1.0)
        cRayNode = CollisionNode('PW.cRayNode')
        cRayNode.addSolid(self.cRay)
        self.cRayNodePath = self.avatarNodePath.attachNewNode(cRayNode)
        self.cRayBitMask = floorBitmask
        cRayNode.setFromCollideMask(self.cRayBitMask)
        cRayNode.setIntoCollideMask(BitMask32.allOff())

        if 0 or self.useLifter:
            # set up floor collision mechanism
            self.lifter = CollisionHandlerFloor()
            self.lifter.setInPattern("enter%in")
            self.lifter.setOutPattern("exit%in")
            self.lifter.setOffset(floorOffset)

            # Limit our rate-of-fall with the lifter.
            # If this is too low, we actually "fall" off steep stairs
            # and float above them as we go down. I increased this
            # from 8.0 to 16.0 to prevent this
            #self.lifter.setMaxVelocity(16.0)

            #self.bobNodePath = self.avatarNodePath.attachNewNode("bob")
            #self.lifter.addCollider(self.cRayNodePath, self.cRayNodePath)
            self.lifter.addCollider(self.cRayNodePath, self.avatarNodePath)
        else: # useCollisionHandlerQueue
            self.cRayQueue = CollisionHandlerQueue()
            self.cTrav.addCollider(self.cRayNodePath, self.cRayQueue)

    def determineHeight(self):
        """
        returns the height of the avatar above the ground.
        If there is no floor below the avatar, 0.0 is returned.
        aka get airborne height.
        """
        if self.useLifter:
            height = self.avatarNodePath.getPos(self.cRayNodePath)
            # If the shadow where not pointed strait down, we would need to
            # get magnitude of the vector.  Since it is strait down, we'll
            # just get the z:
            #spammy --> assert self.debugPrint(
            #spammy -->     "getAirborneHeight() returning %s"%(height.getZ(),))
            assert onScreenDebug.add("height", height.getZ())
            return height.getZ() - self.floorOffset
        else: # useCollisionHandlerQueue
            """
            returns the height of the avatar above the ground.
            If there is no floor below the avatar, 0.0 is returned.
            aka get airborne height.
            """
            height = 0.0
            #*#self.cRayTrav.traverse(render)
            if self.cRayQueue.getNumEntries() != 0:
                # ...we have a floor.
                # Choose the highest of the possibly several floors we're over:
                self.cRayQueue.sortEntries()
                floorPoint = self.cRayQueue.getEntry(0).getFromIntersectionPoint()
                height = -floorPoint.getZ()
            self.cRayQueue.clearEntries()
            if __debug__:
                onScreenDebug.add("height", height)
            return height

    def setupSphere(self, bitmask, avatarRadius):
        """
        Set up the collision sphere
        """
        # This is a sphere on the ground to detect barrier collisions
        if 0:
            self.avatarRadius = avatarRadius
            self.cSphere = CollisionTube(
                Point3(0.0, 0.0, 0.0), Point3(0.0, 40.0, 0.0), avatarRadius)
            cSphereNode = CollisionNode('SP.cSphereNode')
            cSphereNode.addSolid(self.cSphere)
            self.cSphereNodePath = self.avatarNodePath.attachNewNode(cSphereNode)
            self.cSphereBitMask = bitmask
        else:
            # Middle sphere:
            self.avatarRadius = avatarRadius
            #self.cSphere = CollisionSphere(0.0, -5.0, 0.0, avatarRadius)
            #cSphereNode = CollisionNode('SP.cSphereNode')
            #cSphereNode.addSolid(self.cSphere)
            #self.cSphereNodePath = self.avatarNodePath.attachNewNode(cSphereNode)
            self.cSphereBitMask = bitmask

        #cSphereNode.setFromCollideMask(self.cSphereBitMask)
        #cSphereNode.setIntoCollideMask(BitMask32.allOff())

        # set up collision mechanism
        self.pusher = PhysicsCollisionHandler()
        self.pusher.setInPattern("enter%in")
        self.pusher.setOutPattern("exit%in")

        #self.pusher.addCollider(self.cSphereNodePath, self.avatarNodePath)

        if self.useBowSternSpheres:
            # Front sphere:
            self.cBowSphere = CollisionSphere(
                0.0, self.frontSphereOffset, -5.0, avatarRadius)
            cBowSphereNode = CollisionNode('SP.cBowSphereNode')
            cBowSphereNode.addSolid(self.cBowSphere)
            self.cBowSphereNodePath = self.avatarNodePath.attachNewNode(
                cBowSphereNode)

            cBowSphereNode.setFromCollideMask(self.cSphereBitMask)
            cBowSphereNode.setIntoCollideMask(BitMask32.allOff())

            self.cBowSphereNode = cBowSphereNode

            self.pusher.addCollider(
                self.cBowSphereNodePath, self.avatarNodePath)

            # Back sphere:
            self.cSternSphere = CollisionSphere(
                0.0, self.backSphereOffset, -5.0, avatarRadius)
            cSternSphereNode = CollisionNode('SP.cSternSphereNode')
            cSternSphereNode.addSolid(self.cSternSphere)
            self.cSternSphereNodePath = self.avatarNodePath.attachNewNode(
                cSternSphereNode)
            self.cSternSphereBitMask = bitmask

            cSternSphereNode.setFromCollideMask(self.cSphereBitMask)
            cSternSphereNode.setIntoCollideMask(BitMask32.allOff())

            self.pusher.addCollider(
                self.cSternSphereNodePath, self.avatarNodePath)

            # hide other things on my ship that these spheres might collide
            # with and which I dont need anyways...
            shipCollWall = self.avatarNodePath.hull.find("**/collision_hull")
            if not shipCollWall.isEmpty():
                shipCollWall.stash()

    def takedownPhysics(self):
        assert self.debugPrint("takedownPhysics()")
        if hasattr(self, "phys"):
            for i in self.nodes:
                i.removeNode()
            del self.phys
        if self.ship != None:
            self.ship.worldVelocity = Vec3.zero()

    def setupPhysics(self, avatarNodePath):
        assert self.debugPrint("setupPhysics()")
        if avatarNodePath is None:
            return
        assert not avatarNodePath.isEmpty()

        self.takedownPhysics()
        self.nodes = []
        self.phys=PhysicsManager()

        if 0:
            # Connect to Physics Manager:
            self.actorNode=ActorNode("ship-physicsActor")
            self.actorNode.getPhysicsObject().setOriented(1)
            self.actorNode.getPhysical(0).setViscosity(0.1)
            physicsActor=render.attachNewNode(self.actorNode)
            physicsActor.setPos(avatarNodePath, Vec3(0))
            physicsActor.setHpr(avatarNodePath, Vec3(0))
            avatarNodePath.reparentTo(physicsActor)
            avatarNodePath.setPos(Vec3(0))
            avatarNodePath.setHpr(Vec3(0))
            avatarNodePath.assign(physicsActor)
        else:
            physicsActor = avatarNodePath
            self.actorNode = physicsActor.node()
            self.actorNode.getPhysicsObject().setOriented(1)
            self.actorNode.getPhysical(0).setViscosity(0.1)

        fn=ForceNode("ship gravity")
        fnp=NodePath(fn)
        #fnp.reparentTo(physicsActor)
        fnp.reparentTo(render)
        self.nodes.append(fnp)
        gravity=LinearVectorForce(0.0, 0.0, self.__gravity)
        fn.addForce(gravity)
        self.phys.addLinearForce(gravity)
        self.gravity = gravity

        if 0:
            fn=ForceNode("ship buoyancy")
            fnp=NodePath(fn)
            #fnp.reparentTo(physicsActor)
            fnp.reparentTo(render)
            self.nodes.append(fnp)
            buoyancy=LinearVectorForce(0.0, 0.0, 0.0)
            fn.addForce(buoyancy)
            self.phys.addLinearForce(buoyancy)
            self.buoyancy = buoyancy

        fn=ForceNode("ship keel")
        fnp=NodePath(fn)
        #fnp.reparentTo(physicsActor)
        fnp.reparentTo(render)
        self.nodes.append(fnp)
        self.keel=AngularVectorForce(0.0, 0.0, 0.0)
        fn.addForce(self.keel)
        self.phys.addAngularForce(self.keel)

        fn=ForceNode("ship priorParent")
        fnp=NodePath(fn)
        fnp.reparentTo(render)
        self.nodes.append(fnp)
        priorParent=LinearVectorForce(0.0, 0.0, 0.0)
        fn.addForce(priorParent)
        self.phys.addLinearForce(priorParent)
        self.priorParentNp = fnp
        self.priorParent = priorParent

        if 1:
            fn=ForceNode("ship viscosity")
            fnp=NodePath(fn)
            #fnp.reparentTo(physicsActor)
            fnp.reparentTo(render)
            self.nodes.append(fnp)
            self.avatarViscosity=LinearFrictionForce(0.0, 1.0, 0)
            self.avatarViscosity.setCoef(0.5)
            self.avatarViscosity.setAmplitude(2)
            fn.addForce(self.avatarViscosity)
            self.phys.addLinearForce(self.avatarViscosity)

        self.phys.attachLinearIntegrator(LinearEulerIntegrator())
        self.phys.attachAngularIntegrator(AngularEulerIntegrator())
        self.phys.attachPhysicalNode(physicsActor.node())

        if 0:
            self.momentumForce=LinearVectorForce(0.0, 0.0, 0.0)
            fn=ForceNode("ship momentum")
            fnp=NodePath(fn)
            fnp.reparentTo(render)
            self.nodes.append(fnp)
            fn.addForce(self.momentumForce)
            self.phys.addLinearForce(self.momentumForce)

        self.acForce=LinearVectorForce(0.0, 0.0, 0.0)
        self.acForce.setAmplitude(5)
        fn=ForceNode("ship avatarControls")
        fnp=NodePath(fn)
        fnp.reparentTo(render)
        self.nodes.append(fnp)
        fn.addForce(self.acForce)
        self.phys.addLinearForce(self.acForce)
        #self.phys.removeLinearForce(self.acForce)
        #fnp.remove()

        if 0 or self.useHeightRay:
            #self.setupRay(self.floorBitmask, self.avatarRadius)
            self.setupRay(self.floorBitmask, 0.0)


        #avatarNodePath.reparentTo(render)
        self.avatarNodePath = avatarNodePath
        #self.actorNode.getPhysicsObject().resetPosition(self.avatarNodePath.getPos())
        #self.actorNode.updateTransform()
        self.setupSphere(self.wallBitmask|self.floorBitmask, self.avatarRadius)


        assert not avatarNodePath.isEmpty()

        self.setCollisionsActive(1)

    def setWallBitMask(self, bitMask):
        self.wallBitmask = bitMask

    def setFloorBitMask(self, bitMask):
        self.floorBitmask = bitMask

    def initializeCollisions(self, collisionTraverser,
            avatarRadius = 1.4, floorOffset = 1.0, reach = 1.0,
            frontSphereOffset = 35.0, backSphereOffset = -80.0):
        """
        Set up the avatar collisions
        """
        assert self.debugPrint("initializeCollisions()")
        self.cTrav = collisionTraverser
        self.avatarRadius = avatarRadius
        self.floorOffset = floorOffset
        self.reach = reach
        if self.useBowSternSpheres:
            self.frontSphereOffset = frontSphereOffset
            self.backSphereOffset = backSphereOffset

    def deleteCollisions(self):
        assert self.debugPrint("deleteCollisions()")
        del self.cTrav

        if self.useHeightRay:
            del self.cRayQueue
            self.cRayNodePath.removeNode()
            del self.cRayNodePath

        if hasattr(self, "cSphere"):
            del self.cSphere
            self.cSphereNodePath.removeNode()
            del self.cSphereNodePath

            del self.pusher

        self.getAirborneHeight = None

    def setTag(self, key, value):
        if not hasattr(self, "collisionTags"):
            self.collisionTags = {}
        self.collisionTags[key] = value

    def setAirborneHeightFunc(self, getAirborneHeight):
        self.getAirborneHeight = getAirborneHeight

    def setAvatarPhysicsIndicator(self, indicator):
        """
        indicator is a NodePath
        """
        assert self.debugPrint("setAvatarPhysicsIndicator()")
        self.cSphereNodePath.show()
        if indicator:
            # Indicator Node:
            change=render.attachNewNode("change")
            #change.setPos(Vec3(1.0, 1.0, 1.0))
            #change.setHpr(0.0, 0.0, 0.0)
            change.setScale(0.1)
            #change.setColor(Vec4(1.0, 1.0, 1.0, 1.0))
            indicator.reparentTo(change)

            indicatorNode=render.attachNewNode("physVelocityIndicator")
            #indicatorNode.setScale(0.1)
            #indicatorNode.setP(90.0)
            indicatorNode.setPos(self.avatarNodePath, 0.0, 0.0, 6.0)
            indicatorNode.setColor(0.0, 0.0, 1.0, 1.0)
            change.reparentTo(indicatorNode)

            self.physVelocityIndicator=indicatorNode
            # Contact Node:
            contactIndicatorNode=render.attachNewNode("physContactIndicator")
            contactIndicatorNode.setScale(0.25)
            contactIndicatorNode.setP(90.0)
            contactIndicatorNode.setPos(self.avatarNodePath, 0.0, 0.0, 5.0)
            contactIndicatorNode.setColor(1.0, 0.0, 0.0, 1.0)
            indicator.instanceTo(contactIndicatorNode)
            self.physContactIndicator=contactIndicatorNode
        else:
            print "failed load of physics indicator"

    def avatarPhysicsIndicator(self, task):
        #assert self.debugPrint("avatarPhysicsIndicator()")
        # Velocity:
        self.physVelocityIndicator.setPos(self.avatarNodePath, 0.0, 0.0, 6.0)
        physObject=self.actorNode.getPhysicsObject()
        a=physObject.getVelocity()
        self.physVelocityIndicator.setScale(math.sqrt(a.length()))
        a+=self.physVelocityIndicator.getPos()
        self.physVelocityIndicator.lookAt(Point3(a))
        # Contact:
        contact=self.actorNode.getContactVector()
        if contact==Vec3.zero():
            self.physContactIndicator.hide()
        else:
            self.physContactIndicator.show()
            self.physContactIndicator.setPos(self.avatarNodePath, 0.0, 0.0, 5.0)
            #contact=self.actorNode.getContactVector()
            point=Point3(contact+self.physContactIndicator.getPos())
            self.physContactIndicator.lookAt(point)
        return Task.cont

    def setCollisionsActive(self, active = 1):
        assert self.debugPrint("collisionsActive(active=%s)"%(active,))
        if self.collisionsActive != active:
            self.collisionsActive = active
            if active:
                if self.useBowSternSpheres:
                    #self.cTrav.addCollider(self.cSphereNodePath, self.pusher)
                    self.cTrav.addCollider(self.cBowSphereNodePath, self.pusher)
                    self.cTrav.addCollider(self.cSternSphereNodePath, self.pusher)
                if self.useHeightRay:
                    if self.useLifter:
                        self.cTrav.addCollider(self.cRayNodePath, self.lifter)
                    else:
                        self.cTrav.addCollider(self.cRayNodePath, self.cRayQueue)
            else:
                if self.useBowSternSpheres:
                    #self.cTrav.removeCollider(self.cSphereNodePath)
                    self.cTrav.removeCollider(self.cBowSphereNodePath)
                    self.cTrav.removeCollider(self.cSternSphereNodePath)
                if self.useHeightRay:
                    self.cTrav.removeCollider(self.cRayNodePath)
                # Now that we have disabled collisions, make one more pass
                # right now to ensure we aren't standing in a wall.
                self.oneTimeCollide()

    def getCollisionsActive(self):
        assert self.debugPrint(
            "getCollisionsActive() returning=%s"%(self.collisionsActive,))
        return self.collisionsActive

    def placeOnFloor(self):
        """
        Make a reasonable effort to place the avatar on the ground.
        For example, this is useful when switching away from the
        current walker.
        """
        self.oneTimeCollide()
        self.avatarNodePath.setZ(
            self.avatarNodePath.getZ()-self.getAirborneHeight())

    def oneTimeCollide(self):
        """
        Makes one quick collision pass for the avatar, for instance as
        a one-time straighten-things-up operation after collisions
        have been disabled.
        """
        assert self.debugPrint("oneTimeCollide()")
        tempCTrav = CollisionTraverser("oneTimeCollide")
        if self.useHeightRay:
            if self.useLifter:
                tempCTrav.addCollider(self.cRayNodePath, self.lifter)
            else:
                tempCTrav.addCollider(self.cRayNodePath, self.cRayQueue)
        tempCTrav.traverse(render)

    def addBlastForce(self, vector):
        pass

    def displayDebugInfo(self):
        """
        For debug use.
        """
        onScreenDebug.add("w controls", "ShipPilot")

        onScreenDebug.add("w ship", self.ship)
        onScreenDebug.add("w isAirborne", self.isAirborne)
        onScreenDebug.add("posDelta1",
            self.avatarNodePath.getPosDelta(render).pPrintValues())

        physObject=self.actorNode.getPhysicsObject()
        if 0:
            onScreenDebug.add("w posDelta3",
                render.getRelativeVector(
                    self.avatarNodePath,
                    self.avatarNodePath.getPosDelta(render)).pPrintValues())
        if 0:
            onScreenDebug.add("w gravity",
                self.gravity.getLocalVector().pPrintValues())
            onScreenDebug.add("w priorParent",
                self.priorParent.getLocalVector().pPrintValues())

            onScreenDebug.add("w physObject pos",
                physObject.getPosition().pPrintValues())
            onScreenDebug.add("w physObject hpr",
                physObject.getOrientation().getHpr().pPrintValues())
            onScreenDebug.add("w physObject orien",
                physObject.getOrientation().pPrintValues())
        if 1:
            physObject = physObject.getVelocity()
            onScreenDebug.add("w physObject vec",
                physObject.pPrintValues())
            onScreenDebug.add("w physObject len",
                "% 10.4f"%physObject.length())

            acForce = self.acForce.getLocalVector()
            onScreenDebug.add("w acForce vec",
                acForce.pPrintValues())
            onScreenDebug.add("w acForce len",
                "% 10.4f"%acForce.length())

            onScreenDebug.add("w avatarViscosity",
                "% 10.4f"%(self.avatarViscosity.getCoef(),))

            #onScreenDebug.add("physMgr",
            #    self.phys.debugOutput())
            onScreenDebug.add("orientation",
                self.actorNode.getPhysicsObject().getOrientation().pPrintValues())
            #print "ship orientation:", self.actorNode.getPhysicsObject().getOrientation().pPrintValues()

        if 0:
            momentumForce = self.momentumForce.getLocalVector()
            onScreenDebug.add("w momentumForce vec",
                momentumForce.pPrintValues())
            onScreenDebug.add("w momentumForce len",
                "% 10.4f"%momentumForce.length())

        ## if 1:
            ## keel = self.keel.getLocalVector()
            ## onScreenDebug.add("w keel vec",
            ##    keel.pPrintValues())
        if 0:
            onScreenDebug.add("posDelta4",
                self.priorParentNp.getRelativeVector(
                    render,
                    self.avatarNodePath.getPosDelta(render)).pPrintValues())
        if 1:
            onScreenDebug.add("w priorParent",
                self.priorParent.getLocalVector().pPrintValues())
        if 0:
            onScreenDebug.add("w priorParent po",
                self.priorParent.getVector(physObject).pPrintValues())

        if 1:
            onScreenDebug.add("w contact",
                self.actorNode.getContactVector().pPrintValues())
            #onScreenDebug.add("airborneHeight", "% 10.4f"%(
            #    self.getAirborneHeight(),))

    def handleAvatarControls(self, task):
        """
        Check on the arrow keys and update the avatar.
        """
        if __debug__:
            if self.wantDebugIndicator:
                onScreenDebug.append("localAvatar pos = %s\n"%(
                    base.localAvatar.getPos().pPrintValues(),))
                onScreenDebug.append("localAvatar hpr = %s\n"%(
                    base.localAvatar.getHpr().pPrintValues(),))
                #onScreenDebug.append("localAvatar anim = %s\n"%(
                #    base.localAvatar.animFSM.getCurrentState().getName(),))
        #assert self.debugPrint("handleAvatarControls(task=%s)"%(task,))
        physObject=self.actorNode.getPhysicsObject()
        #rotAvatarToPhys=Mat3.rotateMatNormaxis(
        #    -self.avatarNodePath.getH(), Vec3.up())
        #rotPhysToAvatar=Mat3.rotateMatNormaxis(
        #    self.avatarNodePath.getH(), Vec3.up())
        contact=self.actorNode.getContactVector()

        depth=physObject.getPosition().getZ()
        if depth < 0.0:
            # self.buoyancy.setVector(Vec3.zero())
            pass
        else:
            # self.buoyancy.setVector(Vec3.zero())
            physObject.setPosition(Point3(
                physObject.getPosition().getX(),
                physObject.getPosition().getY(),
                0.0))
            self.actorNode.updateTransform()
            #self.buoyancy.setVector(Vec3(0, 0, -depth))

        # get the button states:
        forward = inputState.isSet("forward")
        reverse = inputState.isSet("reverse")
        turnLeft = inputState.isSet("slideLeft") or inputState.isSet("turnLeft")
        turnRight = inputState.isSet("slideRight") or inputState.isSet("turnRight")
        slide = inputState.isSet("slide")
        slideLeft = 0
        slideRight = 0
        jump = inputState.isSet("jump")
        # Determine what the speeds are based on the buttons:

        # this was causing the boat to get stuck moving forward or back
        if 0:
            if not hasattr(self, "sailsDeployed"):
                self.sailsDeployed = 0.0
            if forward and reverse:
                # Way anchor:
                self.__speed = 0.0
                physObject.setVelocity(Vec3.zero())
            elif forward:
                self.sailsDeployed += 0.25
                if self.sailsDeployed > 1.0:
                    self.sailsDeployed = 1.0
            elif reverse:
                self.sailsDeployed -= 0.25
                if self.sailsDeployed < -1.0:
                    self.sailsDeployed = -1.0
            self.__speed = self.ship.acceleration * self.sailsDeployed
        else:
            self.__speed=(forward and self.ship.acceleration or
                    reverse and -self.ship.reverseAcceleration)
        avatarSlideSpeed=self.ship.acceleration*0.5
        #self.__slideSpeed=slide and (
        #        (turnLeft and -avatarSlideSpeed) or
        #        (turnRight and avatarSlideSpeed))
        self.__slideSpeed=(forward or reverse) and (
                (slideLeft and -avatarSlideSpeed) or
                (slideRight and avatarSlideSpeed))
        self.__rotationSpeed=not slide and (
                (turnLeft and self.ship.turnRate) or
                (turnRight and -self.ship.turnRate))


        # Enable debug turbo mode
        maxSpeed = self.ship.maxSpeed
        if __debug__:
            debugRunning = inputState.isSet("debugRunning")
            if debugRunning or base.localAvatar.getTurbo():
                self.__speed*=4.0
                self.__slideSpeed*=4.0
                self.__rotationSpeed*=1.25
                maxSpeed = self.ship.maxSpeed * 4.0


        #*#
        if not hasattr(self, "currentTurning"):
            self.currentTurning = 0.0
        self.currentTurning += self.__rotationSpeed
        if self.currentTurning > self.ship.maxTurn:
            self.currentTurning = self.ship.maxTurn
        elif self.currentTurning < -self.ship.maxTurn:
            self.currentTurning = -self.ship.maxTurn
        if turnLeft or turnRight:
            mult = .9
        elif forward or reverse:
            mult = .82
        else:
            mult = .8
        self.currentTurning *= mult
        if self.currentTurning < 0.001 and self.currentTurning > -0.001:
            self.currentTurning = 0.0
        self.__rotationSpeed = self.currentTurning


        if self.wantDebugIndicator:
            self.displayDebugInfo()

        # How far did we move based on the amount of time elapsed?
        dt=ClockObject.getGlobalClock().getDt()

        if self.needToDeltaPos:
            self.setPriorParentVector()
            self.needToDeltaPos = 0

        airborneHeight=self.getAirborneHeight()
        if airborneHeight > self.highMark:
            self.highMark = airborneHeight
            if __debug__:
                onScreenDebug.add("highMark", "% 10.4f"%(self.highMark,))
        #if airborneHeight < 0.1: #contact!=Vec3.zero():
        if 1:
            if (airborneHeight > self.avatarRadius*0.5
                    or physObject.getVelocity().getZ() > 0.0
                    ): # Check stair angles before changing this.
                # The avatar is airborne (maybe a lot or a tiny amount).
                self.isAirborne = 1
            else:
                # The avatar is very close to the ground (close
                # enough to be considered on the ground).
                if self.isAirborne and physObject.getVelocity().getZ() <= 0.0:
                    # the avatar has landed.
                    contactLength = contact.length()
                    if contactLength>self.__hardLandingForce:
                        messenger.send("jumpHardLand")
                    else:
                        messenger.send("jumpLand")
                    self.priorParent.setVector(Vec3.zero())
                    self.isAirborne = 0
                elif jump:
                    #self.__jumpButton=0
                    messenger.send("jumpStart")
                    if 0:
                        # Jump away from walls and with with the slope normal.
                        jumpVec=Vec3(contact+Vec3.up())
                        #jumpVec=Vec3(rotAvatarToPhys.xform(jumpVec))
                        jumpVec.normalize()
                    else:
                        # Jump straight up, even if next to a wall.
                        jumpVec=Vec3.up()
                    jumpVec*=self.avatarControlJumpForce
                    physObject.addImpulse(Vec3(jumpVec))
                    self.isAirborne = 1 # Avoid double impulse before fully airborne.
                else:
                    self.isAirborne = 0
            if __debug__:
                onScreenDebug.add("isAirborne", "%d"%(self.isAirborne,))
        else:
            if contact!=Vec3.zero():
                # The avatar has touched something (but might
                # not be on the ground).
                contactLength = contact.length()
                contact.normalize()
                angle=contact.dot(Vec3.up())
                if angle>self.__standableGround:
                    # ...avatar is on standable ground.
                    if self.__oldContact==Vec3.zero():
                    #if self.__oldAirborneHeight > 0.1: #self.__oldContact==Vec3.zero():
                        # ...avatar was airborne.
                        self.jumpCount-=1
                        if contactLength>self.__hardLandingForce:
                            messenger.send("jumpHardLand")
                        else:
                            messenger.send("jumpLand")
                    elif jump:
                        self.jumpCount+=1
                        #self.__jumpButton=0
                        messenger.send("jumpStart")
                        jump=Vec3(contact+Vec3.up())
                        #jump=Vec3(rotAvatarToPhys.xform(jump))
                        jump.normalize()
                        jump*=self.avatarControlJumpForce
                        physObject.addImpulse(Vec3(jump))

        if contact!=self.__oldContact:
            # We must copy the vector to preserve it:
            self.__oldContact=Vec3(contact)
        self.__oldAirborneHeight=airborneHeight

        #debugTempH=self.avatarNodePath.getH()
        if __debug__:
            q1=self.avatarNodePath.getQuat()
            q2=physObject.getOrientation()
            q1.normalize()
            q2.normalize()
            assert q1.isSameDirection(q2) or (q1.getHpr() == q2.getHpr())
        assert self.avatarNodePath.getPos().almostEqual(physObject.getPosition(), 0.0001)

        moveToGround = Vec3.zero()
        if not self.useHeightRay or self.isAirborne:
            # ...the airborne check is a hack to stop sliding.
            self.phys.doPhysics(dt)
            if __debug__:
                onScreenDebug.add("phys", "on")
        else:
            physObject.setVelocity(Vec3.zero())
            #if airborneHeight>0.001 and contact==Vec3.zero():
            #    moveToGround = Vec3(0.0, 0.0, -airborneHeight)
            #moveToGround = Vec3(0.0, 0.0, -airborneHeight)
            moveToGround = Vec3(0.0, 0.0, -self.determineHeight())
            if __debug__:
                onScreenDebug.add("phys", "off")

        #debugTempH=self.avatarNodePath.getH()
        if __debug__:
            q1=self.avatarNodePath.getQuat()
            q2=physObject.getOrientation()
            q1.normalize()
            q2.normalize()
            assert q1.isSameDirection(q2) or (q1.getHpr() == q2.getHpr())
        assert self.avatarNodePath.getPos().almostEqual(physObject.getPosition(), 0.0001)

        # Check to see if we're moving at all:
        physVel = physObject.getVelocity()
        physVelLen = physVel.length()
        if (physVelLen!=0.
                or self.__speed
                or self.__slideSpeed
                or self.__rotationSpeed
                or moveToGround!=Vec3.zero()):
            distance = dt * self.__speed
            goForward = True
            if (distance < 0):
                goForward = False
                distance /= 5
            slideDistance = dt * self.__slideSpeed
            rotation = dt * self.__rotationSpeed

            # update pos:
            # Take a step in the direction of our previous heading.
            self.__vel=Vec3(
                Vec3.forward() * distance +
                Vec3.right() * slideDistance)

            # rotMat is the rotation matrix corresponding to
            # our previous heading.
            rotMat=Mat3.rotateMatNormaxis(
                self.avatarNodePath.getH(), Vec3.up())
            step=rotMat.xform(self.__vel)

            #newVector = self.acForce.getLocalVector()+Vec3(step)
            newVector = Vec3(step)
            #newVector=Vec3(rotMat.xform(newVector))
            #maxLen = maxSpeed
            if (goForward):
                maxLen = self.ship.acceleration
            else:
                maxLen = self.ship.reverseAcceleration
            if newVector.length() > maxLen:
                newVector.normalize()
                newVector *= maxLen


            newVector.normalize()
            newVector *= maxLen
            if __debug__:
                onScreenDebug.add("newVector",
                                  newVector)
                onScreenDebug.add("newVector length",
                                  newVector.length())
            self.acForce.setVector(Vec3(newVector))


            #momentum = self.momentumForce.getLocalVector()
            #momentum *= 0.9
            #self.momentumForce.setVector(momentum)


            #physObject.setPosition(Point3(
            #    physObject.getPosition()+step+moveToGround))

            # update hpr:
            o=physObject.getOrientation()
            r=LRotationf()
            r.setHpr(Vec3(rotation, 0.0, 0.0))
            physObject.setOrientation(o*r)

            # sync the change:
            self.actorNode.updateTransform()
            #assert self.avatarNodePath.getH()==debugTempH-rotation
            messenger.send("avatarMoving")
        else:
            # even if there are no active inputs, we still might be moving
            assert physObject.getVelocity().length() == 0.
            self.__vel.set(0.0, 0.0, 0.0)
            goForward = True


        #*#
        speed = physVel
        if (goForward):
            if physVelLen > maxSpeed:
                speed.normalize()
                speed *= maxSpeed
        else:
            if physVelLen > self.ship.maxReverseSpeed:
                speed.normalize()
                speed *= self.ship.maxReverseSpeed

        #speed *= 1.0 - dt * 0.05

        # modify based on sail damage
        speed *= self.ship.Sp
        speed /= self.ship.maxSp
        physObject.setVelocity(speed)

        #rotMat=Mat3.rotateMatNormaxis(self.avatarNodePath.getH(), Vec3.up())
        #speed=rotMat.xform(speed)
        # The momentumForce makes it feel like we are sliding on ice -- Joe
        # f = Vec3(self.__vel)
        # f.normalize()
        # self.momentumForce.setVector(Vec3(f*(speed.length()*0.9)))


        if __debug__:
            q1=self.avatarNodePath.getQuat()
            q2=physObject.getOrientation()
            q1.normalize()
            q2.normalize()
            assert q1.isSameDirection(q2) or q1.getHpr() == q2.getHpr()
        assert self.avatarNodePath.getPos().almostEqual(physObject.getPosition(), 0.0001)

        # Clear the contact vector so we can
        # tell if we contact something next frame
        self.actorNode.setContactVector(Vec3.zero())

        self.__oldPosDelta = self.avatarNodePath.getPosDelta(render)
        self.__oldDt = dt
        assert hasattr(self.ship, 'worldVelocity')
        self.ship.worldVelocity = self.__oldPosDelta*(1/self.__oldDt)
        if self.wantDebugIndicator:
            onScreenDebug.add("w __oldPosDelta vec",
                self.__oldPosDelta.pPrintValues())
            onScreenDebug.add("w __oldPosDelta len",
                "% 10.4f"%self.__oldPosDelta.length())
            onScreenDebug.add("w __oldDt",
                "% 10.4f"%self.__oldDt)
            onScreenDebug.add("w worldVelocity vec",
                self.ship.worldVelocity.pPrintValues())
            onScreenDebug.add("w worldVelocity len",
                "% 10.4f"%self.ship.worldVelocity.length())

        # if hasattr(self.ship, 'sailBillow'):
        #     self.ship.sailBillow = self.sailsDeployed

        if hasattr(self.ship, 'currentTurning'):
            self.ship.currentTurning = self.currentTurning

        return Task.cont

    def doDeltaPos(self):
        assert self.debugPrint("doDeltaPos()")
        self.needToDeltaPos = 1

    def setPriorParentVector(self):
        assert self.debugPrint("doDeltaPos()")

        #print "self.__oldDt", self.__oldDt, "self.__oldPosDelta", self.__oldPosDelta
        if __debug__:
            onScreenDebug.add("__oldDt", "% 10.4f"%self.__oldDt)
            onScreenDebug.add("self.__oldPosDelta",
                              self.__oldPosDelta.pPrintValues())

        velocity = self.__oldPosDelta*(1/self.__oldDt)*4.0 # *4.0 is a hack
        assert self.debugPrint("  __oldPosDelta=%s"%(self.__oldPosDelta,))
        assert self.debugPrint("  velocity=%s"%(velocity,))
        self.priorParent.setVector(Vec3(velocity))
        if __debug__:
            if self.wantDebugIndicator:
                onScreenDebug.add("velocity", velocity.pPrintValues())

    def reset(self):
        assert self.debugPrint("reset()")
        self.actorNode.getPhysicsObject().resetPosition(
            self.avatarNodePath.getPos())
        self.priorParent.setVector(Vec3.zero())
        self.highMark = 0
        self.actorNode.setContactVector(Vec3.zero())
        if __debug__:
            contact=self.actorNode.getContactVector()
            onScreenDebug.add("priorParent po", self.priorParent.getVector(
                self.actorNode.getPhysicsObject()).pPrintValues())
            onScreenDebug.add("highMark", "% 10.4f"%(self.highMark,))
            onScreenDebug.add("contact", contact.pPrintValues())

    def getVelocity(self):
        return self.__vel

    def enableAvatarControls(self):
        """
        Activate the arrow keys, etc.
        """
        assert self.debugPrint("enableAvatarControls()")
        assert self.collisionsActive

        if __debug__:
            #self.accept("control-f3", self.spawnTest) #*#
            self.accept("f3", self.reset) # for debugging only.

        taskName = "AvatarControls-%s"%(id(self),)
        # remove any old
        taskMgr.remove(taskName)
        # spawn the new task
        taskMgr.add(self.handleAvatarControls, taskName, 25)
        if self.physVelocityIndicator:
            taskMgr.add(
                self.avatarPhysicsIndicator,
                "AvatarControlsIndicator%s"%(id(self),), 35)

    def disableAvatarControls(self):
        """
        Ignore the arrow keys, etc.
        """
        assert self.debugPrint("disableAvatarControls()")
        taskName = "AvatarControls-%s"%(id(self),)
        taskMgr.remove(taskName)

        taskName = "AvatarControlsIndicator%s"%(id(self),)
        taskMgr.remove(taskName)

        if __debug__:
            self.ignore("control-f3") #*#
            self.ignore("f3")

    if __debug__:
        def setupAvatarPhysicsIndicator(self):
            if self.wantDebugIndicator:
                indicator=loader.loadModelCopy('phase_5/models/props/dagger')
                #self.walkControls.setAvatarPhysicsIndicator(indicator)

        def debugPrint(self, message):
            """for debugging"""
            return self.notify.debug(
                    str(id(self))+' '+message)
