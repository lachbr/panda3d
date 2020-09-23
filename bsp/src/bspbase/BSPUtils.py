"""BSPUtils module: Various helper/utility functions"""

from panda3d.core import deg2Rad, rad2Deg, Vec3, VBase4, NodePath

import math

def vec3LinearToGamma(vec):
    """Converts a vector from linear space to gamma space"""
    vec[0] = math.pow(vec[0], 1.0 / 2.2)
    vec[1] = math.pow(vec[1], 1.0 / 2.2)
    vec[2] = math.pow(vec[2], 1.0 / 2.2)
    return vec

def vec3GammaToLinear(vec):
    """Converts a vector from gamma space to linear space"""
    vec[0] = math.pow(vec[0], 2.2)
    vec[1] = math.pow(vec[1], 2.2)
    vec[2] = math.pow(vec[2], 2.2)
    return vec

def colorFromRGBScalar255(color):
    """Takes in a tuple of (r, g, b, scalar) (0-255) and returns a
    linear (0-1) color with the scalar applied."""

    scalar = color[3]
    return VBase4(color[0] * (scalar / 255.0) / 255.0,
                  color[1] * (scalar / 255.0) / 255.0,
                  color[2] * (scalar / 255.0) / 255.0,
                  1.0)

def reflect(dir, normal):
    """Returns a reflection vector given an incident vector and surface normal"""
    return dir - (normal * (dir.dot(normal) * 2.0))

def extrude(start, scale, direction):
    """Returns a vector extruded in `direction` by a factor of `scale` with the
    offset of `start`"""
    return start + (direction * scale)

def angleVectors(angles, forward = False, right = False, up = False):
    """
    Get basis vectors for the angles.
    Each vector is optional.

    Brian: I don't know if this is even used anywhere, nor if it even works
    correcty.
    """

    if forward or right or up:
        sh = math.sin(deg2Rad(angles[0]))
        ch = math.cos(deg2Rad(angles[0]))
        sp = math.sin(deg2Rad(angles[1]))
        cp = math.cos(deg2Rad(angles[1]))
        sr = math.sin(deg2Rad(angles[2]))
        cr = math.cos(deg2Rad(angles[2]))

    result = []
    if forward:
        forward = Vec3(cp*ch,
                       cp*sh,
                       -sp)
        result.append(forward)
    if right:
        right = Vec3(-1*sr*sp*ch+-1*cr*-sh,
                     -1*sr*sp*sh+-1*cr*ch,
                     -1*sr*cp)
        result.append(right)
    if up:
        up = Vec3(cr*sp*ch+-sr*-sh,
                  cr*sp*sh+-sr*ch,
                  cr*cp)
        result.append(up)

    return result

def vecToYaw(vec):
    """Converts the given vector to a yaw angle in degrees."""
    return rad2Deg(math.atan2(vec[1], vec[0])) - 90

def angleMod(a):
    """Constrains the given angle to be within 0-360"""
    return a % 360

def angleDiff(destAngle, srcAngle):
    """Returns the difference between two angles, correctly flipping it around
    if need be."""

    delta = destAngle - srcAngle

    if destAngle > srcAngle:
        if delta >= 180:
            delta -= 360
    else:
        if delta <= -180:
            delta += 360

    return delta

def remapVal(val, A, B, C, D):
    """Remaps the specified value in range of A-B to be in range of C-D"""
    if A == B:
        return D if val >= B else C

    return C + (D - C) * (val - A) / (B - A)

def clamp(val, A, B):
    """Clamps the given value to to be >= A and <= B"""
    return max(A, min(B, val))

def anglesToVector(angles):
    """Converts a set of HPR Euler angles to a forward vector.

    NOTE: Roll cannot be represented in a direction vector, so that does not
    factor into the result."""
    return Vec3(math.cos(deg2Rad(angles[0])) * math.cos(deg2Rad(angles[1])),
                math.sin(deg2Rad(angles[0])) * math.cos(deg2Rad(angles[1])),
                math.sin(deg2Rad(angles[1])))

def lerpWithRatio(goal, val, ratio):
    dt = globalClock.getDt()
    amt = 1 - math.pow(1 - ratio, dt * 30.0)
    return lerp(goal, val, amt)

def lerp(v0, v1, amt):
    return v0 * amt + v1 * (1 - amt)

def lerp2(a, b, c):
    """Returns a value between `a` and `b` using `c` as a bias factor."""
    return a + ((b - a) * c)

def isNodePathOk(np):
    """Returns True if the specified NodePath is not None and not empty, False
    otherwise."""
    return (np is not None and not np.isEmpty())

def areFacingEachOther(obj1, obj2):
    """Returns True if two nodes are facing each other like so: -> <-"""
    h1 = obj1.getH(NodePath()) % 360
    h2 = obj2.getH(NodePath()) % 360
    return not (-90.0 <= (h1 - h2) <= 90.0)

def makePulseEffectInterval(guiElement, defScale, minScale, maxScale, d1, d2):
    from direct.interval.IntervalGlobal import Sequence, LerpScaleInterval
    seq = Sequence()
    seq.append(LerpScaleInterval(guiElement, d1, maxScale, minScale, blendType = 'easeOut'))
    seq.append(LerpScaleInterval(guiElement, d2, defScale, maxScale, blendType = 'easeInOut'))
    return seq

######################################################
# String helper functions

def makeSingular(noun):
    """ Makes a plural noun singular. """
    pluralSuffixes = ['ies', 'es', 's']

    for suffix in pluralSuffixes:
        if noun.endswith(suffix):
            return noun[:-len(suffix)]

    return noun

def makePlural(noun):
    """ Makes a noun string plural. Follows grammar rules with nouns ending in 'y' and 's'. Assumes noun is singular beforehand. """
    withoutLast = noun[:-1]

    if noun.endswith('y'):
        return '{0}ies'.format(withoutLast)
    elif noun.endswith('s'):
        return '{0}es'.format(withoutLast)
    else:
        return '{0}s'.format(noun)

def makePastTense(noun):
    """ Makes a noun string past tense. """
    withoutLast = noun[:-1]
    secondToLast = noun[-2:]
    lastChar = noun[-1:]

    if noun.endswith('y'):
        return '{0}ied'.format(withoutLast)
    elif not (noun.endswith('w') or noun.endswith('y')) and secondToLast in getVowels() and lastChar in getConsonants():
        # When a noun ends with a vowel then a consonant, we double the consonant and add -ed."
        return '{0}{1}ed'.format(noun, secondToLast)
    else:
        return '{0}ed'.format(noun)

def getVowels():
    """ Returns a list of vowels """
    return ['a', 'e', 'i', 'o', 'u']

def getConsonants():
    """ Returns a list of consonants """
    return ['b', 'c', 'd', 'f', 'g',
            'h', 'j', 'k', 'l', 'm',
            'n', 'p', 'q', 'r', 's',
            't', 'v', 'w', 'x', 'y',
            'z']

def getAmountString(noun, amount):
    """ Returns a grammatically correct string stating the amount of something. E.g: An elephant horn, 5 packages, etc. """

    if amount > 1:
        return "{0} {1}".format(amount, makePlural(noun))
    else:
        firstChar = noun[1:]

        if firstChar in getVowels():
            # If the noun begins with a vowel, we use 'an'.
            return 'An {0}'.format(noun)

        return 'A {0}'.format(noun)

def isEmptyString(string):
    """ Returns whether or not a string is empty. """
    return not (string and string.strip())


######################################################
