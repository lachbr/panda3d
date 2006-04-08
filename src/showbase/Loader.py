"""Loader module: contains the Loader class"""

from pandac.PandaModules import *
from direct.directnotify.DirectNotifyGlobal import *

# You can specify a phaseChecker callback to check
# a modelPath to see if it is being loaded in the correct
# phase
phaseChecker = None

class Loader:
    """
    Load models, textures, sounds, and code.
    """
    notify = directNotify.newCategory("Loader")
    modelCount = 0

    # special methods
    def __init__(self, base):
        self.base = base
        self.loader = PandaLoader()

    def destroy(self):
        del self.base
        del self.loader

    # model loading funcs
    def loadModel(self, modelPath, fMakeNodeNamesUnique = 0,
                  loaderOptions = None):
        """
        modelPath is a string.

        Attempt to load a model from given file path, return
        a nodepath to the model if successful or None otherwise.
        """
        assert Loader.notify.debug("Loading model: %s" % (modelPath))
        if phaseChecker:
            phaseChecker(modelPath)
        if loaderOptions == None:
            loaderOptions = LoaderOptions()
        node = self.loader.loadSync(Filename(modelPath), loaderOptions)
        if (node != None):
            nodePath = NodePath(node)
            if fMakeNodeNamesUnique:
                self.makeNodeNamesUnique(nodePath, 0)
        else:
            nodePath = None
        return nodePath

    def loadModelOnce(self, modelPath):
        """
        modelPath is a string.

        Attempt to load a model from modelPool, if not present
        then attempt to load it from disk. Return a nodepath to
        the model if successful or None otherwise
        """
        assert Loader.notify.debug("Loading model once: %s" % (modelPath))
        if phaseChecker:
            phaseChecker(modelPath)
        node = ModelPool.loadModel(modelPath)
        if (node != None):
            nodePath = NodePath.anyPath(node)
        else:
            nodePath = None
        return nodePath

    def loadModelOnceUnder(self, modelPath, underNode):
        """loadModelOnceUnder(self, string, string | node | NodePath)
        Behaves like loadModelOnce, but also implicitly creates a new
        node to attach the model under, which helps to differentiate
        different instances.

        underNode may be either a node name, or a NodePath or a Node
        to an already-existing node.

        This is useful when you want to load a model once several
        times before parenting each instance somewhere, or when you
        want to load a model and immediately set a transform on it.
        But also consider loadModelCopy().
        """
        assert Loader.notify.debug(
            "Loading model once: %s under %s" % (modelPath, underNode))
        if phaseChecker:
            phaseChecker(modelPath)
        node = ModelPool.loadModel(modelPath)
        if (node != None):
            nodePath = NodePath(underNode)
            nodePath.attachNewNode(node)
        else:
            nodePath = None
        return nodePath

    def loadModelCopy(self, modelPath):
        """loadModelCopy(self, string)
        Attempt to load a model from modelPool, if not present
        then attempt to load it from disk. Return a nodepath to
        a copy of the model if successful or None otherwise
        """
        assert Loader.notify.debug("Loading model copy: %s" % (modelPath))
        if phaseChecker:
            phaseChecker(modelPath)
        node = ModelPool.loadModel(modelPath)
        if (node != None):
            return NodePath(node.copySubgraph())
        else:
            return None

    def loadModelNode(self, modelPath):
        """
        modelPath is a string.

        This is like loadModelOnce in that it loads a model from the
        modelPool, but it does not then instance it to hidden and it
        returns a Node instead of a NodePath.  This is particularly
        useful for special models like fonts that you don't care about
        where they're parented to, and you don't want a NodePath
        anyway--it prevents accumulation of instances of the font
        model under hidden.

        However, if you're loading a font, see loadFont(), below.
        """
        assert Loader.notify.debug("Loading model once node: %s" % (modelPath))
        if phaseChecker:
            phaseChecker(modelPath)
        return ModelPool.loadModel(modelPath)

    def unloadModel(self, modelPath):
        """
        modelPath is a string.
        """
        assert Loader.notify.debug("Unloading model: %s" % (modelPath))
        ModelPool.releaseModel(modelPath)

    # font loading funcs
    def loadFont(self, modelPath,
                 spaceAdvance = None, pointSize = None,
                 pixelsPerUnit = None, scaleFactor = None,
                 textureMargin = None, polyMargin = None,
                 minFilter = None, magFilter = None,
                 anisotropicDegree = None,
                 lineHeight = None):
        """
        modelPath is a string.

        This loads a special model as a TextFont object, for rendering
        text with a TextNode.  A font file must be either a special
        egg file (or bam file) generated with egg-mkfont, or a
        standard font file (like a TTF file) that is supported by
        FreeType.
        """
        assert Loader.notify.debug("Loading font: %s" % (modelPath))
        if phaseChecker:
            phaseChecker(modelPath)

        font = FontPool.loadFont(modelPath)
        if font == None:
            # If we couldn't load the model, at least return an
            # empty font.
            font = StaticTextFont(PandaNode("empty"))

        # The following properties may only be set for dynamic fonts.
        if hasattr(font, "setPointSize"):
            if pointSize != None:
                font.setPointSize(pointSize)
            if pixelsPerUnit != None:
                font.setPixelsPerUnit(pixelsPerUnit)
            if scaleFactor != None:
                font.setScaleFactor(scaleFactor)
            if textureMargin != None:
                font.setTextureMargin(textureMargin)
            if polyMargin != None:
                font.setPolyMargin(polyMargin)
            if minFilter != None:
                font.setMinFilter(minFilter)
            if magFilter != None:
                font.setMagFilter(magFilter)
            if anisotropicDegree != None:
                font.setAnisotropicDegree(anisotropicDegree)

        if lineHeight is not None:
            # If the line height is specified, it overrides whatever
            # the font itself thinks the line height should be.  This
            # and spaceAdvance should be set last, since some of the
            # other parameters can cause these to be reset to their
            # default.
            font.setLineHeight(lineHeight)

        if spaceAdvance is not None:
            font.setSpaceAdvance(spaceAdvance)

        return font

    # texture loading funcs
    def loadTexture(self, texturePath, alphaPath = None,
                    readMipmaps = False):
        """
        texturePath is a string.

        Attempt to load a texture from the given file path using
        TexturePool class. Returns None if not found
        """
        if alphaPath is None:
            assert Loader.notify.debug("Loading texture: %s" % (texturePath))
            if phaseChecker:
                phaseChecker(texturePath)
            texture = TexturePool.loadTexture(texturePath, 0, readMipmaps)
        else:
            assert Loader.notify.debug("Loading texture: %s %s" % (texturePath, alphaPath))
            if phaseChecker:
                phaseChecker(texturePath)
            texture = TexturePool.loadTexture(texturePath, alphaPath, 0, 0, readMipmaps)
        return texture

    def load3DTexture(self, texturePattern, readMipmaps = False):
        """
        texturePattern is a string that contains a sequence of one or
        more '#' characters, which will be filled in with the sequence
        number.

        Returns a 3-D Texture object, suitable for rendering
        volumetric textures, if successful, or None if not.
        """
        assert Loader.notify.debug("Loading 3-D texture: %s" % (texturePattern))
        if phaseChecker:
            phaseChecker(texturePattern)
        texture = TexturePool.load3dTexture(texturePattern, readMipmaps)
        return texture

    def loadCubeMap(self, texturePattern, readMipmaps = False):
        """
        texturePattern is a string that contains a sequence of one or
        more '#' characters, which will be filled in with the sequence
        number.

        Returns a six-face cube map Texture object if successful, or
        None if not.

        """
        assert Loader.notify.debug("Loading cube map: %s" % (texturePattern))
        if phaseChecker:
            phaseChecker(texturePattern)
        texture = TexturePool.loadCubeMap(texturePattern, readMipmaps)
        return texture

    def unloadTexture(self, texture):

        """
        Removes the previously-loaded texture from the cache, so
        that when the last reference to it is gone, it will be
        released.  This also means that the next time the same texture
        is loaded, it will be re-read from disk (and duplicated in
        texture memory if there are still outstanding references to
        it).

        The texture parameter may be the return value of any previous
        call to loadTexture(), load3DTexture(), or loadCubeMap().
        """
        assert Loader.notify.debug("Unloading texture: %s" % (texture))
        TexturePool.releaseTexture(texture)

    # sound loading funcs
    def loadSfx(self, name):
        assert Loader.notify.debug("Loading sound: %s" % (name))
        if phaseChecker:
            phaseChecker(name)
        # should return a valid sound obj even if soundMgr is invalid
        sound = None
        if (name):
            # showbase-created sfxManager should always be at front of list
            sound=base.sfxManagerList[0].getSound(name)
        if sound == None:
            Loader.notify.warning("Could not load sound file %s." % name)
        return sound

    def loadMusic(self, name):
        assert Loader.notify.debug("Loading sound: %s" % (name))
        # should return a valid sound obj even if musicMgr is invalid
        sound = None
        if (name):
            sound=base.musicManager.getSound(name)
        if sound == None:
            Loader.notify.warning("Could not load music file %s." % name)
        return sound


    def makeNodeNamesUnique(self, nodePath, nodeCount):
        if nodeCount == 0:
            Loader.modelCount += 1
        nodePath.setName(nodePath.getName() +
                         ('_%d_%d' % (Loader.modelCount, nodeCount)))
        for i in range(nodePath.getNumChildren()):
            nodeCount += 1
            self.makeNodeNamesUnique(nodePath.getChild(i), nodeCount)

    def loadShader (self, shaderPath):
        shader = ShaderPool.loadShader (shaderPath)
        if (shader == None):
            Loader.notify.warning("Could not load shader file %s." % shaderPath)
        return shader

    def unloadShader(self, shaderPath):
        if (shaderPath != None):
            ShaderPool.releaseShader(shaderPath)
        