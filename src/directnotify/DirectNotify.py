
"""DirectNotify module: this module contains the DirectNotify class"""

import Notifier
import Logger

class DirectNotify:
    """DirectNotify class: this class contains methods for creating
    mulitple notify categories via a dictionary of Notifiers."""
    
    def __init__(self):
        """__init__(self)
        DirectNotify class keeps a dictionary of Notfiers"""
        self.__categories = { }
        # create a default log file
        self.logger = Logger.Logger()

    def __str__(self):
        """__str__(self)
        Print handling routine"""
        return "DirectNotify categories: %s" % (self.__categories)

    #getters and setters
    def getCategories(self):
        """getCategories(self)
        Return list of category dictionary keys"""
        return (self.__categories.keys())

    def getCategory(self, categoryName):
        """getCategory(self, string)
        Return the category with given name if present, None otherwise"""
        return (self.__categories.get(categoryName, None))

    def newCategory(self, categoryName, logger=None):
        """newCategory(self, string)
        Make a new notify category named categoryName. Return new category
        if no such category exists, else return existing category"""
        if (not self.__categories.has_key(categoryName)):
            self.__categories[categoryName] = Notifier.Notifier(categoryName, logger)
            self.setDconfigLevel(categoryName)
        else:
            print "Warning: DirectNotify: category '%s' already exists" % \
                  (categoryName)
        return (self.getCategory(categoryName))

    def setDconfigLevel(self, categoryName):
        """
        Check to see if this category has a dconfig variable
        to set the notify severity and then set that level. You cannot
        set these until config is set.
        """

        # We cannot check dconfig variables until config has been
        # set. Once config is set in ShowBase.py, it tries to set
        # all the levels again in case some were created before config
        # was created.
        try:
            config
        except:
            return 0
        
        dconfigParam = ("notify-level-" + categoryName)
        level = config.GetString(dconfigParam, "")
        if level:
            print ("Setting DirectNotify category: " + dconfigParam +
                   " to severity: " + level)
            category = self.getCategory(categoryName)
            if level == "error":
                category.setWarning(0)
                category.setInfo(0)
                category.setDebug(0)
            elif level == "warning":
                category.setWarning(1)
                category.setInfo(0)
                category.setDebug(0)
            elif level == "info":
                category.setWarning(1)
                category.setInfo(1)
                category.setDebug(0)
            elif level == "debug":
                category.setWarning(1)
                category.setInfo(1)
                category.setDebug(1)
            else:
                print ("DirectNotify: unknown notify level: " + str(level)
                       + " for category: " + str(categoryName))
            
    def setDconfigLevels(self):
        for categoryName in self.getCategories():
            self.setDconfigLevel(categoryName)




