"""
The DoInterestManager keeps track of which parent/zones that we currently
have interest in.  When you want to "look" into a zone you add an interest
to that zone.  When you want to get rid of, or ignore, the objects in that
zone, remove interest in that zone.

p.s. A great deal of this code is just code moved from ClientRepository.py.
"""

from pandac.PandaModules import *
from MsgTypes import *
from direct.showbase.PythonUtil import *
from direct.showbase import DirectObject
from PyDatagram import PyDatagram
from direct.directnotify.DirectNotifyGlobal import directNotify

# indices into interest table entries
DESC  = 0
STATE = 1
SCOPE = 2
EVENT = 3

STATE_ACTIVE = 'Active'
STATE_PENDING_DEL = 'PendingDel'

# scope value for interest changes that have no complete event
NO_SCOPE = 0

class DoInterestManager(DirectObject.DirectObject):
    """
    Top level Interest Manager
    """
    notify = directNotify.newCategory("DoInterestManager")
    try:
        tempbase = base
    except:
        tempbase = simbase
    InterestDebug = tempbase.config.GetBool('interest-debug', True)
    del tempbase

    # 'handle' is a number that represents a single interest set that the
    # client has requested; the interest set may be modified
    _HandleSerialNum = 0
    # high bit is reserved for server interests
    _HandleMask = 0x7FFF

    # 'scope' refers to a single request to change an interest set
    _ScopeIdSerialNum = 100
    _ScopeIdMask = 0x3FFFFFFF # avoid making Python create a long

    _interests = {}
    if __debug__:
        _debug_currentInterests = {}
        _debug_interestHistory = []

    def __init__(self):
        assert DoInterestManager.notify.debugCall()
        DirectObject.DirectObject.__init__(self)

    def addInterest(self, parentId, zoneIdList, description, event=None):
        """
        Look into a (set of) zone(s).
        """
        assert DoInterestManager.notify.debugCall()
        handle = self._getNextHandle()
        if event is not None:
            scopeId = self._getNextScopeId()
        else:
            scopeId = NO_SCOPE
        DoInterestManager._interests[handle] = [
            description, STATE_ACTIVE, scopeId, event]
        if self.InterestDebug:
            print 'INTEREST DEBUG: addInterest(): handle=%s, parent=%s, zoneIds=%s, description=%s, event=%s' % (
                handle, parentId, zoneIdList, description, event)
        self._sendAddInterest(handle, scopeId, parentId, zoneIdList)
        assert self.printInterestsIfDebug()
        return handle

    def removeInterest(self, handle, event=None):
        """
        Stop looking in a (set of) zone(s)
        """
        assert DoInterestManager.notify.debugCall()
        existed = False
        if DoInterestManager._interests.has_key(handle):
            existed = True
            if event is not None:
                scopeId = self._getNextScopeId()
            else:
                scopeId = NO_SCOPE
            DoInterestManager._interests[handle][STATE] = STATE_PENDING_DEL
            DoInterestManager._interests[handle][SCOPE] = scopeId
            DoInterestManager._interests[handle][EVENT] = event
            if self.InterestDebug:
                print 'INTEREST DEBUG: removeInterest(): handle=%s, event=%s' % (
                    handle, event)
            self._sendRemoveInterest(handle, scopeId)
        else:
            DoInterestManager.notify.warning(
                "removeInterest: handle not found: %s" % (handle))
        assert self.printInterestsIfDebug()
        return existed

    def alterInterest(self, handle, parentId, zoneIdList, description=None,
            event=None):
        """
        Removes old interests and adds new interests.

        Note that when an interest is changed, only the most recent
        change's event will be triggered. Previous events are abandoned.
        If this is a problem, consider opening multiple interests.
        """
        assert DoInterestManager.notify.debugCall()
        exists = False
        if DoInterestManager._interests.has_key(handle):
            if description is not None:
                DoInterestManager._interests[handle][DESC] = description

            if event is not None:
                scopeId = self._getNextScopeId()
            else:
                scopeId = NO_SCOPE
            DoInterestManager._interests[handle][SCOPE] = scopeId
            DoInterestManager._interests[handle][EVENT] = event

            if self.InterestDebug:
                print 'INTEREST DEBUG: alterInterest(): handle=%s, parent=%s, zoneIds=%s, description=%s, event=%s' % (
                    handle, parentId, zoneIdList, description, event)
            self._sendAddInterest(handle, scopeId, parentId, zoneIdList)
            exists = True
            assert self.printInterestsIfDebug()
        else:
            DoInterestManager.notify.warning(
                "alterInterest: handle not found: %s" % (handle))
        return exists

    def _getNextHandle(self):
        handle = DoInterestManager._HandleSerialNum
        while True:
            handle = (handle + 1) & DoInterestManager._HandleMask
            # skip handles that are already in use
            if handle not in DoInterestManager._interests:
                break
            DoInterestManager.notify.warning(
                'interest %s already in use' % handle)
        DoInterestManager._HandleSerialNum = handle
        return DoInterestManager._HandleSerialNum
    def _getNextScopeId(self):
        scopeId = DoInterestManager._ScopeIdSerialNum
        while True:
            scopeId = (scopeId + 1) & DoInterestManager._ScopeIdMask
            # skip over the 'no scope' id
            if scopeId != NO_SCOPE:
                break
        DoInterestManager._ScopeIdSerialNum = scopeId
        return DoInterestManager._ScopeIdSerialNum

    def _considerRemoveInterest(self, handle):
        """
        Consider whether we should cull the interest set.
        """
        assert DoInterestManager.notify.debugCall()
        if DoInterestManager._interests.has_key(handle):
            if DoInterestManager._interests[handle][STATE]==STATE_PENDING_DEL:
                # make sure there is no pending event for this interest
                if DoInterestManager._interests[handle][SCOPE] == NO_SCOPE:
                    del DoInterestManager._interests[handle]

    if __debug__:
        def printInterestsIfDebug(self):
            if DoInterestManager.notify.getDebug():
                self.printInterests()
            return 1 # for assert

        def printInterests(self):
            print "***************** Interest History *************"
            format = '%9s %6s %6s %9s %s'
            print format % (
                "Action", "Handle", "Scope", "ParentId",
                "ZoneIdList")
            for i in DoInterestManager._debug_interestHistory:
                print format % tuple(i)
            print "Note: interests with a Scope of 0 do not get" \
                " done/finished notices."
            print "******************* Interest Sets **************"
            format = '%6s %20s %10s %8s %5s %9s %9s %s'
            print format % (
                "Handle", "Description", "State", "Scope", "Event", 
                "ScopeId", "ParentId", "ZoneIdList")
            for id, data in DoInterestManager._interests.items():
                print format % tuple((id,) + tuple(data) + \
                    DoInterestManager._debug_currentInterests.get(id))
            print "************************************************"

    def _sendAddInterest(self, handle, scopeId, parentId, zoneIdList):
        """
        Part of the new otp-server code.

        handle is a client-side created number that refers to
                a set of interests.  The same handle number doesn't
                necessarily have any relationship to the same handle
                on another client.
        """
        assert DoInterestManager.notify.debugCall()
        if __debug__:
            if isinstance(zoneIdList, types.ListType):
                zoneIdList.sort()
            DoInterestManager._debug_currentInterests[handle]=(
                scopeId, parentId, zoneIdList)
            DoInterestManager._debug_interestHistory.append(
                ("add", handle, scopeId, parentId, zoneIdList))
        if parentId == 0:
            DoInterestManager.notify.error(
                'trying to set interest to invalid parent: %s' % parentId)
        datagram = PyDatagram()
        # Add message type
        datagram.addUint16(CLIENT_ADD_INTEREST)
        datagram.addUint16(handle)
        datagram.addUint32(scopeId)
        datagram.addUint32(parentId)
        if isinstance(zoneIdList, types.ListType):
            vzl = list(zoneIdList)
            vzl.sort()
            uniqueElements(vzl)
            for zone in vzl:
                datagram.addUint32(zone)
        else:
           datagram.addUint32(zoneIdList)
        self.send(datagram)

    def _sendRemoveInterest(self, handle, scopeId):
        """
        handle is a client-side created number that refers to
                a set of interests.  The same handle number doesn't
                necessarily have any relationship to the same handle
                on another client.
        """
        assert DoInterestManager.notify.debugCall()
        datagram = PyDatagram()
        # Add message type
        datagram.addUint16(CLIENT_REMOVE_INTEREST)
        datagram.addUint16(handle)
        if scopeId != 0:
            datagram.addUint32(scopeId)
        self.send(datagram)
        if __debug__:
            DoInterestManager._debug_interestHistory.append(
                ("remove", handle, scopeId) +
                DoInterestManager._debug_currentInterests.get(handle)[1:])

    def handleInterestDoneMessage(self, di):
        """
        This handles the interest done messages and may dispatch an event
        """
        assert DoInterestManager.notify.debugCall()
        handle = di.getUint16()
        scopeId = di.getUint32()
        if self.InterestDebug:
            print 'INTEREST DEBUG: interestDone(): handle=%s' % handle
        DoInterestManager.notify.debug(
            "handleInterestDoneMessage--> Received handle %s, scope %s" % (
            handle, scopeId))
        # if the scope matches, send out the event
        if scopeId == DoInterestManager._interests[handle][SCOPE]:
            event = DoInterestManager._interests[handle][EVENT]
            DoInterestManager._interests[handle][SCOPE] = NO_SCOPE
            DoInterestManager._interests[handle][EVENT] = None
            DoInterestManager.notify.debug(
                "handleInterestDoneMessage--> Sending event %s" % event)
            messenger.send(event)
        else:
            DoInterestManager.notify.warning(
                "handleInterestDoneMessage--> Expecting scope %s, got %s" % (
                DoInterestManager._interests[handle][SCOPE], scopeId))
        self._considerRemoveInterest(handle)
        if __debug__:
            DoInterestManager._debug_interestHistory.append(
                ("finished", handle, scopeId) +
                DoInterestManager._debug_currentInterests.get(handle)[1:])

        assert self.printInterestsIfDebug()
