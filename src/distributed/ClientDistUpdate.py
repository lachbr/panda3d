"""ClientDistUpdate module: contains the ClientDistUpdate class"""

import DirectNotifyGlobal
import Datagram
from MsgTypes import *

# These are stored here so that the distributed classes we load on the fly
# can be exec'ed in the module namespace as if we imported them normally.
# This is important for redefine to work, and is a good idea anyways.
moduleGlobals = globals()
moduleLocals = locals()

class ClientDistUpdate:
    notify = DirectNotifyGlobal.directNotify.newCategory("ClientDistUpdate")

    def __init__(self, cdc, dcField):
        self.cdc = cdc
        self.field = dcField
        self.number = dcField.getNumber()
        self.name = dcField.getName()
        self.types = []
        self.divisors = []
        self.deriveTypesFromParticle(dcField)
        # Figure out our function pointer
        exec("import " + cdc.name, moduleGlobals, moduleLocals)
        try:
            self.func = eval(cdc.name + "." + cdc.name + "." + self.name)
        # Only catch name and attribute errors
        # as all other errors are legit errors
        except (NameError, AttributeError), e:
            #ClientDistUpdate.notify.warning(cdc.name + "." + self.name +
            #                                " does not exist")
            self.func = None
        return None

    def deriveTypesFromParticle(self, dcField):
        dcFieldAtomic = dcField.asAtomicField()
        dcFieldMolecular = dcField.asMolecularField()
        if dcFieldAtomic:
            for i in range(0, dcFieldAtomic.getNumElements()):
                self.types.append(dcFieldAtomic.getElementType(i))
                self.divisors.append(dcFieldAtomic.getElementDivisor(i))
        elif dcFieldMolecular:
            for i in range(0, dcFieldMolecular.getNumAtomics()):
                componentField = dcFieldMolecular.getAtomic(i)
                for j in range(0, componentField.getNumElements()):
                    self.types.append(componentField.getElementType(j))
                    self.divisors.append(componentField.getElementDivisor(j))
        else:
            ClientDistUpdate.notify.error("field is neither atom nor molecule")
        return None

    def updateField(self, cdc, do, di):
        # Get the arguments into a list
        args = self.extractArgs(di)
        # Apply the function to the object with the arguments
        if self.func != None:
            apply(self.func, [do] + args)
        return None

    def extractArgs(self, di):
        args = []
        assert(len(self.types) == len(self.divisors))
        numTypes = len(self.types)
        for i in range(numTypes):
            args.append(di.getArg(self.types[i], self.divisors[i]))
        return args

    def addArgs(self, datagram, args):
        # Add the args to the datagram
        numElems = len(args)
        assert (numElems == len(self.types) == len(self.divisors))
        for i in range(0, numElems):
            datagram.putArg(args[i], self.types[i], self.divisors[i])
    
    def sendUpdate(self, cr, do, args, sendToId = None):
        assert(self.notify.debug("sendUpdate() name="+str(self.name)))
        datagram = Datagram.Datagram()
        # Add message type
        datagram.addUint16(CLIENT_OBJECT_UPDATE_FIELD)
        # Add the DO id
        if sendToId == None:
            datagram.addUint32(do.doId)
        else:
            datagram.addUint32(sendToId)
        # Add the field id
        datagram.addUint16(self.number)
        # Add the arguments
        self.addArgs(datagram, args)
        # send the datagram
        cr.send(datagram)
