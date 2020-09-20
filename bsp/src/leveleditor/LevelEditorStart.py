import sys

try:
    import PyQt5
except ImportError:
    print("ERROR: You need to pull in PyQt5 via pip to use the level editor")
    sys.exit(1)

try:
    import py_linq
except ImportError:
    print("ERROR: You need to pull in py_linq via pip to use the level editor")
    sys.exit(1)

from PyQt5 import QtWidgets, QtGui

import builtins
from src.coginvasion.base.Metadata import Metadata
metadata = Metadata()
builtins.metadata = metadata

from panda3d.core import loadPrcFile, loadPrcFileData, ConfigVariableString, ConfigVariableDouble
loadPrcFile('config/Confauto.prc')
loadPrcFile('config/config_client.prc')
loadPrcFileData('', 'model-path ./resources') # Don't require mounting of phases
loadPrcFileData('', 'model-path ./hl2/materials')
loadPrcFileData('', 'model-path ./tf/materials')
loadPrcFileData('', 'window-type none')

from src.coginvasion.settings.SettingsManager import SettingsManager
from src.coginvasion.settings.Setting import SHOWBASE_PREINIT, SHOWBASE_POSTINIT
jsonFile = "settings.json"

# Initialize settings manager, but don't actually load any settings.
# The settings don't matter to the editor, but I need to define it
# because there might be code that relies on this being defined.
sm = SettingsManager()

from src.coginvasion.globals import CIGlobals
CIGlobals.SettingsMgr = sm

from src.leveleditor.LevelEditor import LevelEditor
base = LevelEditor()

ConfigVariableDouble('decompressor-step-time').setValue(0.01)
ConfigVariableDouble('extractor-step-time').setValue(0.01)

print("taha")

base.run()
