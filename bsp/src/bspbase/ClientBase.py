from .HostBase import HostBase
from direct.directnotify.DirectNotifyGlobal import directNotify
from direct.distributed2.ClientRepository import ClientRepository

from panda3d.core import GraphicsEngine, GraphicsPipeSelection

class ClientBase(HostBase):
    """ Main routine for client process """

    notify = directNotify.newCategory("ClientBase")

    def __init__(self):
        HostBase.__init__(self)

        # Initialize rendering things
        self.graphicsEngine = GraphicsEngine.getGlobalPtr()
        self.pipeSelection = GraphicsPipeSelection.getGlobalPtr()
        self.pipe = self.pipeSelection.makeDefaultPipe()
        self.win = None

        # Camera things
        self.camera = None
        self.cam = None
        self.camLens = None
        self.camNode = None

        # Scene things
        self.render = None
        self.render2d = None
        self.aspect2d = None
        self.hidden = None

        self.cl = self.createClientRepository()

    def createClientRepository(self):
        return ClientRepository()
