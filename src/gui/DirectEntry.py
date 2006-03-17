from DirectFrame import *
import types

# DirectEntry States:
ENTRY_FOCUS_STATE    = PGEntry.SFocus      # 0
ENTRY_NO_FOCUS_STATE = PGEntry.SNoFocus    # 1
ENTRY_INACTIVE_STATE = PGEntry.SInactive   # 2

class DirectEntry(DirectFrame):
    """
    DirectEntry(parent) - Create a DirectGuiWidget which responds
    to keyboard buttons
    """

    directWtext = ConfigVariableBool('direct-wtext', 1)

    def __init__(self, parent = None, **kw):
        # Inherits from DirectFrame
        # A Direct Frame can have:
        # - A background texture (pass in path to image, or Texture Card)
        # - A midground geometry item (pass in geometry)
        # - A foreground text Node (pass in text string or Onscreen Text)
        # For a direct entry:
        # Each button has 3 states (focus, noFocus, disabled)
        # The same image/geom/text can be used for all three states or each
        # state can have a different text/geom/image
        # State transitions happen automatically based upon mouse interaction
        optiondefs = (
            # Define type of DirectGuiWidget
            ('pgFunc',          PGEntry,          None),
            ('numStates',       3,                None),
            ('state',           NORMAL,           None),
            ('entryFont',       None,             INITOPT),
            ('width',           10,               self.setup),
            ('numLines',        1,                self.setup),
            ('focus',           0,                self.setFocus),
            ('cursorKeys',      1,                self.setCursorKeysActive),
            ('obscured',        0,                self.setObscureMode),
            # Setting backgroundFocus allows the entry box to get keyboard
            # events that are not handled by other things (i.e. events that
            # fall through to the background):
            ('backgroundFocus', 0,                self.setBackgroundFocus),
            # Text used for the PGEntry text node
            # NOTE: This overrides the DirectFrame text option
            ('initialText',     '',               INITOPT),
            # Command to be called on hitting Enter
            ('command',        None,              None),
            ('extraArgs',      [],                None),
            # commands to be called when focus is gained or lost
            ('focusInCommand', None,              None),
            ('focusInExtraArgs', [],              None),
            ('focusOutCommand', None,              None),
            ('focusOutExtraArgs', [],              None),
            # Sounds to be used for button events
            ('rolloverSound',   getDefaultRolloverSound(), self.setRolloverSound),
            ('clickSound',    getDefaultClickSound(),    self.setClickSound),
            )
        # Merge keyword options with default options
        self.defineoptions(kw, optiondefs)

        # Initialize superclasses
        DirectFrame.__init__(self, parent)

        if self['entryFont'] == None:
            font = getDefaultFont()
        else:
            font = self['entryFont']

        # Create Text Node Component
        self.onscreenText = self.createcomponent(
            'text', (), None,
            OnscreenText,
            (), parent = hidden,
            # Pass in empty text to avoid extra work, since its really
            # The PGEntry which will use the TextNode to generate geometry
            text = '',
            # PGEntry assumes left alignment
            align = TextNode.ALeft,
            font = font,
            scale = 1,
            # Don't get rid of the text node
            mayChange = 1)

        # We can get rid of the node path since we're just using the
        # onscreenText as an easy way to access a text node as a
        # component
        self.onscreenText.removeNode()

        # Bind command function
        self.bind(ACCEPT, self.commandFunc)

        self.accept(self.guiItem.getFocusInEvent(), self.focusInCommandFunc)
        self.accept(self.guiItem.getFocusOutEvent(), self.focusOutCommandFunc)

        # Call option initialization functions
        self.initialiseoptions(DirectEntry)

        # Update TextNodes for each state
        for i in range(self['numStates']):
            self.guiItem.setTextDef(i, self.onscreenText.textNode)

        # Update initial text
        self.unicodeText = 0
        if self['initialText']:
            self.set(self['initialText'])

    def destroy(self):
        self.ignore(self.guiItem.getFocusInEvent())
        self.ignore(self.guiItem.getFocusOutEvent())
        DirectFrame.destroy(self)

    def setup(self):
        self.node().setup(self['width'], self['numLines'])

    def setFocus(self):
        PGEntry.setFocus(self.guiItem, self['focus'])

    def setCursorKeysActive(self):
        PGEntry.setCursorKeysActive(self.guiItem, self['cursorKeys'])

    def setObscureMode(self):
        PGEntry.setObscureMode(self.guiItem, self['obscured'])

    def setBackgroundFocus(self):
        PGEntry.setBackgroundFocus(self.guiItem, self['backgroundFocus'])

    def setRolloverSound(self):
        rolloverSound = self['rolloverSound']
        if rolloverSound:
            self.guiItem.setSound(ENTER + self.guiId, rolloverSound)
        else:
            self.guiItem.clearSound(ENTER + self.guiId)

    def setClickSound(self):
        clickSound = self['clickSound']
        if clickSound:
            self.guiItem.setSound(ACCEPT + self.guiId, clickSound)
        else:
            self.guiItem.clearSound(ACCEPT + self.guiId)

    def commandFunc(self, event):
        if self['command']:
            # Pass any extra args to command
            apply(self['command'], [self.get()] + self['extraArgs'])

    def focusInCommandFunc(self):
        if self['focusInCommand']:
            apply(self['focusInCommand'], self['focusInExtraArgs'])

    def focusOutCommandFunc(self):
        if self['focusOutCommand']:
            apply(self['focusOutCommand'], self['focusOutExtraArgs'])

    def set(self, text):
        self.unicodeText = isinstance(text, types.UnicodeType)
        if self.unicodeText:
            self.guiItem.setWtext(text)
        else:
            self.guiItem.setText(text)

    def get(self):
        if not self.directWtext.getValue():
            # If the user has configured wide-text off, then always
            # return an 8-bit string.  This will be encoded if
            # necessary, according to Panda's default encoding.
            return self.guiItem.getText()

        if self.unicodeText:
            return self.guiItem.getWtext()
        else:
            # Although we weren't expecting a wide character, the user
            # might give us one.  If this happens, we have to return a
            # wide string.
            wtext = self.guiItem.getWtext()
            text = self.guiItem.getText()
            try:
                matches = (wtext == unicode(text))
            except:
                return wtext
            if matches:
                return text
            else:
                return wtext

    def setCursorPosition(self, pos):
        if (pos < 0):
            self.guiItem.setCursorPosition(len(self.get()) + pos)
        else:
            self.guiItem.setCursorPosition(pos)

    def enterText(self, text):
        """ sets the entry's text, and moves the cursor to the end """
        self.set(text)
        self.setCursorPosition(len(self.get()))

    def getBounds(self, state = 0):
        # Compute the width and height for the entry itself, ignoring
        # geometry etc.
        tn = self.onscreenText.textNode
        mat = tn.getTransform()
        align = tn.getAlign()
        lineHeight = tn.getLineHeight()
        numLines = self['numLines']
        width = self['width']

        if align == TextNode.ALeft:
            left = 0.0
            right = width
        elif align == TextNode.ACenter:
            left = -width / 2.0
            right = width / 2.0
        elif align == TextNode.ARight:
            left = -width
            right = 0.0

        bottom = -0.3 * lineHeight - (lineHeight * (numLines - 1))
        top = lineHeight

        self.ll.set(left, 0.0, bottom)
        self.ur.set(right, 0.0, top)
        self.ll = mat.xformPoint(self.ll)
        self.ur = mat.xformPoint(self.ur)

        # Scale bounds to give a pad around graphics.  We also want to
        # scale around the border width.
        pad = self['pad']
        borderWidth = self['borderWidth']
        self.bounds = [self.ll[0] - pad[0] - borderWidth[0],
                       self.ur[0] + pad[0] + borderWidth[0],
                       self.ll[2] - pad[1] - borderWidth[1],
                       self.ur[2] + pad[1] + borderWidth[1]]
        return self.bounds
