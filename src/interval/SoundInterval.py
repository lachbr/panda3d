"""SoundInterval module: contains the SoundInterval class"""

from PandaModules import *
from DirectNotifyGlobal import *
import Interval

class SoundInterval(Interval.Interval):
    # Name counter
    soundNum = 1
    # create SoundInterval DirectNotify category
    notify = directNotify.newCategory('SoundInterval')
    # Class methods
    # Create a sound interval
    # If loop = 0, sound will play once, duration of the interval
    # equals the duration of the sound
    # If loop = 1, the sound will loop for the specified duration
    # If no duration is specified, sound will loop for the duration
    # of the sound, i.e. it will only play once.....usually, there
    # seems to be some timing in the audio such that the stop doesn't
    # kill the looping sound until the next time around if duration
    # of the interval equals duration of the sound
    def __init__(self, sound, loop = 0, duration = 0.0, name = None, volume = 1.0, startTime = 0.0):
        """__init__(sound, loop, name)
        """
        # Generate unique name
        id = 'Sound-%d' % SoundInterval.soundNum
        SoundInterval.soundNum += 1
        # Record instance variables
        self.sound = sound
        self.loop = loop
        self.volume = volume
        self.startTime = startTime
        # If no duration given use sound's duration as interval's duration
        if duration == 0.0 and self.sound != None:
            duration = max(self.sound.length() - self.startTime, 0)
            if (duration == 0):
                self.notify.warning('zero length duration!')
            # MPG - hack for Miles bug
            #duration += 1.5
            # DCR - hack for Miles bug - adding 1.5 seconds caused
            # problems for MG_neg_buzzer.wav
            duration += min(duration * 2.4, 1.5)
        # Generate unique name if necessary
        if (name == None):
            name = id
        # Initialize superclass
        Interval.Interval.__init__(self, name, duration)

    def privInitialize(self, t):
        # If its within a 10th of a second of the start,
        # start at the beginning
        t1 = t + self.startTime
        if (t1 < 0.1):
            t1 = 0.0
        if self.sound != None:
            self.sound.setVolume(self.volume)
            self.sound.setTime(t1)
            self.sound.setLoop(self.loop)
            self.sound.play()
        self.state = CInterval.SStarted
        self.currT = t1

    def privStep(self, t):
        if self.state == CInterval.SPaused:
            # Restarting from a pause.
            if self.sound != None:
                self.sound.setVolume(self.volume)
                self.sound.setTime(t)
                self.sound.setLoop(self.loop)
                self.sound.play()
        self.state = CInterval.SStarted
        self.currT = t

    def privFinalize(self):
        if self.sound != None:
            self.sound.stop()
        self.currT = self.getDuration()
        self.state = CInterval.SFinal

    def privInterrupt(self):
        if self.sound != None:
            self.sound.stop()
        self.state = CInterval.SPaused
