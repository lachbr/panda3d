"""LEConfig: Contains the level editor configuration variables"""

from panda3d.core import ConfigVariableList, ConfigVariableString, ConfigVariableDouble, ConfigVariableInt

fgd_files = ConfigVariableList("fgd-file")
default_point_entity = ConfigVariableString("default-point-entity", "prop_static")
default_solid_entity = ConfigVariableString("default-solid-entity", "func_wall")
default_texture_scale = ConfigVariableDouble("default-texture-scale", 1.0)
default_lightmap_scale = ConfigVariableInt("default-lightmap-scale", 16)
