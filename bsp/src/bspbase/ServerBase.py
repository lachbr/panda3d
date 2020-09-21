from panda3d.core import PStatClient

from .HostBase import HostBase
from direct.distributed2.ServerRepository import ServerRepository
from direct.distributed2.ServerConfig import sv_port

from direct.directnotify.DirectNotifyGlobal import directNotify

class ServerBase(HostBase):
    """ Main rountine for server process """

    notify = directNotify.newCategory("ServerBase")

    def __init__(self):
        HostBase.__init__(self)

        self.sv = self.createServerRepository()

    def createServerRepository(self):
        return ServerRepository(sv_port.getValue())
