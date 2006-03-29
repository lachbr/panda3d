
from MessengerGlobal import messenger

class DirectObject:
    """
    This is the class that all Direct/SAL classes should inherit from
    """
    def __init__(self):
        pass

    def __del__(self):
        self.ignoreAll()
        # This next line is useful for debugging leaks
        #print "Destructing: ", self.__class__.__name__

    # Wrapper functions to have a cleaner, more object oriented approach to
    # the messenger functionality.

    def accept(self, event, method, extraArgs=[]):
        return messenger.accept(event, self, method, extraArgs, 1)

    def acceptOnce(self, event, method, extraArgs=[]):
        return messenger.accept(event, self, method, extraArgs, 0)

    def ignore(self, event):
        return messenger.ignore(event, self)

    def ignoreAll(self):
        return messenger.ignoreAll(self)

    def isAccepting(self, event):
        return messenger.isAccepting(event, self)

    def isIgnoring(self, event):
        return messenger.isIgnoring(event, self)
