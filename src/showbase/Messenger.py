
from PythonUtil import *
from DirectNotifyGlobal import *


class Messenger:

    notify = None    

    def __init__(self):
        """ __init__(self)
        One dictionary does it all. It has the following structure:
            {event1 : {object1: [method, extraArgs, persistent],
                       object2: [method, extraArgs, persistent]},
             event2 : {object1: [method, extraArgs, persistent],
                       object2: [method, extraArgs, persistent]}}

        Or, for an example with more real data:
            {'mouseDown' : {avatar : [avatar.jump, [2.0], 1]}}
        """
        self.dict = {}

        if (Messenger.notify == None):
            Messenger.notify = directNotify.newCategory("Messenger")

        # Messenger.notify.setDebug(1)
            
        
    def accept(self, event, object, method, extraArgs=[], persistent=1):
        """ accept(self, string, DirectObject, Function, List, Boolean)
        
        Make this object accept this event. When the event is
        sent (using Messenger.send or from C++), method will be executed,
        optionally passing in extraArgs.
        
        If the persistent flag is set, it will continue to respond
        to this event, otherwise it will respond only once.
        """

        Messenger.notify.debug('object: ' + `object`
                  + '\n now accepting: ' + `event`
                  + '\n method: ' + `method`
                  + '\n extraArgs: ' + `extraArgs`
                  + '\n persistent: ' + `persistent`)
            
        acceptorDict = ifAbsentPut(self.dict, event, {})
        acceptorDict[object] = [method, extraArgs, persistent]

    def ignore(self, event, object):
        """ ignore(self, string, DirectObject)
        Make this object no longer respond to this event.
        It is safe to call even if it was not alread
        """

        Messenger.notify.debug(`object` + '\n now ignoring: ' + `event`)
            
        if self.dict.has_key(event):
            # Find the dictionary of all the objects accepting this event
            acceptorDict = self.dict[event]
            # If this object is there, delete it from the dictionary
            if acceptorDict.has_key(object):
                del acceptorDict[object]
            # If this dictionary is now empty, remove the event
            # entry from the Messenger alltogether
            if (len(acceptorDict) == 0):
                del self.dict[event]


    def ignoreAll(self, object):
        """ ignoreAll(self, DirectObject)
        Make this object no longer respond to any events it was accepting
        Useful for cleanup
        """

        Messenger.notify.debug(`object` + '\n now ignoring all events')

        for event in self.dict.keys():
            # Find the dictionary of all the objects accepting this event
            acceptorDict = self.dict[event]
            # If this object is there, delete it from the dictionary
            if acceptorDict.has_key(object):
                del acceptorDict[object]
            # If this dictionary is now empty, remove the event
            # entry from the Messenger alltogether
            if (len(acceptorDict) == 0):
                del self.dict[event]


    def isAccepting(self, event, object):
        """ isAccepting(self, string, DirectOject)        
        Is this object accepting this event?
        """
        if self.dict.has_key(event):
            if self.dict[event].has_key(object):
                # Found it, return true
                return 1
                
        # If we looked in both dictionaries and made it here
        # that object must not be accepting that event.
        return 0
        
    def isIgnoring(self, event, object):
        """ isIgnorning(self, string, DirectObject)
        Is this object ignoring this event?
        """
        return (not self.isAccepting(event, object))

    def send(self, event, sentArgs=[]):
        """ send(self, string, [arg1, arg2,...])
        Send this event, optionally passing in arguments
        """
        
        # Do not print the new frame debug, it is too noisy!
        if (event != 'NewFrame'):
            Messenger.notify.debug('sent event: ' + event + ' sentArgs: ' + `sentArgs`)

        if self.dict.has_key(event):
            acceptorDict = self.dict[event]
            for object in acceptorDict.keys():
                # We have to make this apparently redundant check, because
                # it is possible that one object removes its own hooks
                # in response to a handler called by a previous object.
                if acceptorDict.has_key(object):
                    method, extraArgs, persistent = acceptorDict[object]
                    apply(method, (extraArgs + sentArgs))
                    # If this object was only accepting this event once,
                    # remove it from the dictionary
                    if not persistent:
                        # We need to check this because the apply above might
                        # have done an ignore.
                        if acceptorDict.has_key(object):
                            del acceptorDict[object]
                        # If this dictionary is now empty, remove the event
                        # entry from the Messenger alltogether
                        if ((len(acceptorDict) == 0) and
                            (self.dict.has_key(event))):
                            del self.dict[event]

    def clear(self):
        """clear(self)
        Start fresh with a clear dict
        """
        self.dict.clear()

        
    def replaceMethod(self, oldMethod, newFunction):
        """
        This is only used by Finder.py - the module that lets
        you redefine functions with Control-c-Control-v
        """
        import new
        for entry in self.dict.items():
            event, objectDict = entry
            for objectEntry in objectDict.items():
                object, params = objectEntry
                method = params[0]
                if (type(method) == types.MethodType):
                    function = method.im_func
                else:
                    function = method
                #print ('function: ' + `function` + '\n' +
                #       'method: ' + `method` + '\n' +
                #       'oldMethod: ' + `oldMethod` + '\n' +
                #       'newFunction: ' + `newFunction` + '\n')
                if (function == oldMethod):
                    newMethod = new.instancemethod(newFunction,
                                                   method.im_self,
                                                   method.im_class)
                    params[0] = newMethod
                    # Found it retrun true
                    return 1
        # didnt find that method, return false
        return 0
    
    def toggleVerbose(self):
        Messenger.notify.setDebug(1 - Messenger.notify.getDebug())

    def __repr__(self):
        """
        Compact version of event, acceptor pairs
        """
        str = "="*64 + "\n"
        keys = self.dict.keys()
        keys.sort()
        for event in keys:
            str = str + event.ljust(32) + '\t'
            acceptorDict = self.dict[event]
            for object in acceptorDict.keys():
                method, extraArgs, persistent = acceptorDict[object]
                className = object.__class__.__name__
                methodName = method.__name__
                str = str + className + '.' + methodName + ' '
            str = str + '\n'
        return str

    def detailedRepr(self):
        """
        Print out the table in a detailed readable format
        """
        import types
        str = 'Messenger\n'
        str = str + '='*50 + '\n'
        keys = self.dict.keys()
        keys.sort()
        for event in keys:
            acceptorDict = self.dict[event]
            str = str + 'Event: ' + event + '\n'
            for object in acceptorDict.keys():
                function, extraArgs, persistent = acceptorDict[object]
                if (type(object) == types.InstanceType):
                    className = object.__class__.__name__
                else:
                    className = "Not a class"
                functionName = function.__name__
                str = (str + '\t' +
                       'Acceptor:     ' + className + ' instance' + '\n\t' +
                       'Function name:' + functionName + '\n\t' +
                       'Extra Args:   ' + `extraArgs` + '\n\t' +
                       'Persistent:   ' + `persistent` + '\n')
                # If this is a class method, get its actual function
                if (type(function) == types.MethodType):
                    str = (str + '\t' +
                           'Method:       ' + `function` + '\n\t' +
                           'Function:     ' + `function.im_func` + '\n')
                else:
                    str = (str + '\t' +
                           'Function:     ' + `function` + '\n')
        str = str + '='*50 + '\n'
        return str

