from PandaObject import *
from ClusterClient import *
import string

# A dictionary of information for various cluster configurations.
# Dictionary is keyed on cluster-config string
# Each dictionary contains a list of display configurations, one for
# each display in the cluster
# Information that can be specified for each display:
#      display name: Name of display (used in Configrc to specify server)
#      display type: Used to flag client vs. server
#      pos:   positional offset of display's camera from main cluster group
#      hpr:   orientation offset of display's camera from main cluster group
#      focal length: display's focal length (in mm)
#      film size: display's film size (in inches)
#      film offset: offset of film back (in inches)
# Note: Note, this overrides offsets specified in DirectCamConfig.py
# For now we only specify frustum for first display region of configuration
# TODO: Need to handle multiple display regions per cluster node and to
# generalize to non cluster situations

ClientConfigs = {
    'single-server'       : [{'display name' : 'display0',
                              'pos' : Vec3(0),
                              'hpr' : Vec3(0)}
                             ],
    'two-server'          : [{'display name' : 'display0',
                              'display mode' : 'client',
                              'pos' : Vec3(0),
                              'hpr' : Vec3(-30,0,0)},
                             {'display name' : 'display1',
                              'pos' : Vec3(0),
                              'hpr' : Vec3(30,0,0)}
                             ],
    'mono-modelcave-pipe0': [{'display name' : 'display0',
                              'pos' : Vec3(0),
                              'hpr' : Vec3(0)},
                             {'display name' : 'display1',
                              'pos' : Vec3(0),
                              'hpr' : Vec3(0)}
                             ],
    'cavetestsmall'   : [{'display name' : 'la',
                          'pos' : Vec3(-0.105, -0.020, 5.000),
                          'hpr' : Vec3(51.213, 0.000, 0.000),
                          'focal length' : 0.809,
                          'film size' : (1.000, 0.831),
                          'film offset' : (0.000, 0.173),
                          },
                         {'display name' : 'lb',
                          'display mode' : 'client',
                          'pos' : Vec3(-0.105, -0.020, 5.000),
                          'hpr' : Vec3(-0.370, 0.000, 0.000),
                          'focal length' : 0.815,
                          'film size' : (1.000, 0.831),
                          'film offset' : (0.000, 0.173),
                          },
                         {'display name' : 'lc',
                          'pos' : Vec3(-0.105, -0.020, 5.000),
                          'hpr' : Vec3(-51.675, 0.000, 0.000),
                          'focal length' : 0.820,
                          'film size' : (1.000, 0.830),
                          'film offset' : (-0.000, 0.173),
                          },
                         ],
    'cavetest'        : [{'display name' : 'la',
                          'pos' : Vec3(-0.105, -0.020, 5.000),
                          'hpr' : Vec3(51.213, 0.000, 0.000),
                          'focal length' : 0.809,
                          'film size' : (1.000, 0.831),
                          'film offset' : (0.000, 0.173),
                          },
                         {'display name' : 'lb',
                          'display mode' : 'client',
                          'pos' : Vec3(-0.105, -0.020, 5.000),
                          'hpr' : Vec3(-0.370, 0.000, 0.000),
                          'focal length' : 0.815,
                          'film size' : (1.000, 0.831),
                          'film offset' : (0.000, 0.173),
                          },
                         {'display name' : 'lc',
                          'pos' : Vec3(-0.105, -0.020, 5.000),
                          'hpr' : Vec3(-51.675, 0.000, 0.000),
                          'focal length' : 0.820,
                          'film size' : (1.000, 0.830),
                          'film offset' : (-0.000, 0.173),
                          },
                         {'display name' : 'ra',
                          'pos' : Vec3(0.105, -0.020, 5.000),
                          'hpr' : Vec3(51.675, 0.000, 0.000),
                          'focal length' : 0.820,
                          'film size' : (1.000, 0.830),
                          'film offset' : (0.000, 0.173),
                          },
                         {'display name' : 'rb',
                          'pos' : Vec3(0.105, -0.020, 5.000),
                          'hpr' : Vec3(0.370, 0.000, 0.000),
                          'focal length' : 0.815,
                          'film size' : (1.000, 0.831),
                          'film offset' : (0.000, 0.173),
                          },
                         {'display name' : 'rc',
                          'pos' : Vec3(0.105, -0.020, 5.000),
                          'hpr' : Vec3(-51.213, 0.000, 0.000),
                          'focal length' : 0.809,
                          'film size' : (1.000, 0.831),
                          'film offset' : (-0.000, 0.173),
                          },
                         ],
    }

