
"""
Type Descriptors

Type Descriptors are used for code generation of C++ types. They
know everything they need to know about themselves to generate code.
They get constructed by and stored in FFIInterrogateDatabase.

"""

import sys
import os
import string
import FFIConstants
import FFIOverload


from PythonUtil import *

class BaseTypeDescriptor:
    """
    A type descriptor contains everything you need to know about a C++ function,
    class, or primitive.
    """
    def __init__(self):
        # The pythonified name from C++
        self.foreignTypeName = ''

        # The typeIndex for lookup in the typeIndexMap
        self.typeIndex = 0

        # The C++ prototype for this type
        self.prototype = ''

        # The C++ comment for this type
        self.comment = ''

        # Is this a nested type?
        self.isNested = 0

        # If we are nested, this is the typeDescriptor we are nested in
        self.outerType = None

        # The type descriptors for the types we derive from
        self.parentTypes = []
        
        # atomicType may be one of the following
        # AT_not_atomic = 0
        # AT_int = 1
        # AT_float = 2
        # AT_double = 3
        # AT_bool = 4
        # AT_char = 5
        # AT_void = 6
        # AT_string = 7
        # By default this type is not atomic
        self.atomicType = 0

    def isAtomic(self):
        return (self.atomicType != 0)
        
    def generateGlobalCode(self, dir, extensionsDir):
        # By default generate no code
        pass
    def recursiveTypeDescriptor(self):
        """
        Attempt to get to the bottom of a type descriptor
        Since we are at the bottom when we get here, just return self
        """
        return self
    def recordOverloadedMethods(self):
        # By default do nothing
        pass
    def generateReturnValueWrapper(self, file, userManagesMemory,
                                   needsDowncast, nesting):
        # By default do nothing
        pass
    def getFullNestedName(self):
        """
        If this type is nested, it will return the fully specified name
        For example:  OuterClass.InnerClass.ReallyInnerClass
        """
        if self.isNested:
            return self.outerType.getFullNestedName() + '.' + self.foreignTypeName
        else:
            return self.foreignTypeName


class PrimitiveTypeDescriptor(BaseTypeDescriptor):
    """
    Primitive type descriptors include int, float, char, etc.
    These get mapped to Python types like IntType, FloatType, StringType
    """
    def __init__(self):
        BaseTypeDescriptor.__init__(self)
                
    def generateReturnValueWrapper(self, file, userManagesMemory,
                                   needsDowncast, nesting):
        """
        Write code to the file that will return a primitive to the caller.
        Pretty simple since there is no extra work needed here
        """
        indent(file, nesting, 'return returnValue\n')



class EnumTypeDescriptor(PrimitiveTypeDescriptor):
    """
    EnumTypeDescriptors represent enums in C++
    """
    def __init__(self):
        PrimitiveTypeDescriptor.__init__(self)
        # A dictionary of name, value pairs for this enum
        self.values = {}
        # The enum name is different than the foreignTypeName because
        # we record the foreignTypeName as enum (int)
        self.enumName = ''
        # Specify that we do not have any parent or nested types to make
        # the sorting based on inheritance happy. Essentially, we do not
        # inherit from anybody or have any nested types
        self.parentTypes = []
        self.nestedTypes = []

    def generateGlobalCode(self, dir, extensionsDir):
        """
        Generate enum code for this type.
        """
        fileName = self.enumName + '.py'
        file = open(os.path.join(dir, fileName), 'w')
        indent(file, 0, FFIConstants.generatedHeader)
        self.generateCode(file, 0)

    def generateCode(self, file, nesting):
        self.outputComment(file, nesting)
        self.outputValues(file, nesting)
        

    def outputComment(self, file, nesting):
        indent(file, nesting, '\n')
        indent(file, nesting, '##################################################\n')
        indent(file, nesting, '#  Enum ' + self.enumName + '\n')
        indent(file, nesting, '##################################################\n')
        indent(file, nesting, '\n')

    def outputValues(self, file, nesting):
        """
        For each entry in the dictionary, output a line for name, value pairs
        Example:
        off = 0
        on = 1
        """
        for key in self.values.keys():
            indent(file, nesting, key + ' = ' + `self.values[key]` + '\n')
            

class DerivedTypeDescriptor(BaseTypeDescriptor):
    """
    DerivedTypeDescriptor is a wrapper around a primitive or class type
    For instance const, or pointer to.
    """
    def __init__(self):
        BaseTypeDescriptor.__init__(self)
        self.typeDescriptor = None
        
    def recursiveTypeDescriptor(self):
        """
        Attempt to get to the bottom of a type descriptor by
        recursively unravelling typeDescriptors until you get to
        a type that is not derived (primitive or class) in which
        case the base class will just return self.
        """
        return self.typeDescriptor.recursiveTypeDescriptor()

class PointerTypeDescriptor(DerivedTypeDescriptor):
    """
    Points to another type descriptor
    """
    def __init__(self):
        DerivedTypeDescriptor.__init__(self)

class ConstTypeDescriptor(DerivedTypeDescriptor):
    """
    Const version of another type descriptor
    """
    def __init__(self):
        DerivedTypeDescriptor.__init__(self)

class ClassTypeDescriptor(BaseTypeDescriptor):
    """
    This describes a C++ class. It holds lists of all its methods too.
    It can also generate Python shadow class code for itself.
    """
    def __init__(self):
        BaseTypeDescriptor.__init__(self)
        
        # Methods interrogate told us were constructors
        self.constructors = []
        
        # A method interrogate told us is the destructor
        self.destructor = None
        
        # Methods interrogate told us were instance methods
        # Note: the methods without the this pointer get moved into staticMethods
        self.instanceMethods = []
        
        # Methods interrogate told us were upcast methods
        self.upcastMethods = []
        
        # Methods interrogate told us were downcast methods
        self.downcastMethods = []
        
        # Instance methods that had no this pointer are moved into here
        self.staticMethods = []
        
        # Global methods that take this class as the first parameter are just
        # stored with the class because it is more useable that way
        self.globalMethods = []

        # These are dictionaries used to temporarily hold methods for
        # overloading while generating code
        self.overloadedClassMethods = {}
        self.overloadedInstanceMethods = {}

        # Nested typeDescriptors inside this class
        self.nestedTypes = []

    def getExtensionModuleName(self):
        """
        Return a filename for the extensions for this class
        Example: NodePath extensions would be found in NodePath-extensions.py
        """
        return self.foreignTypeName + '-extensions.py'

    def getCModules(self):
        """
        Return a list of all the C modules this class references
        """
        try:
            # Prevent from doing the work twice
            # if CModules is already defined, just return it
            return self.CModules
        except:
            # Otherwise, it must be our first time through, do the real work
            self.CModules = []
            for method in (self.constructors + [self.destructor] + self.instanceMethods
                           + self.upcastMethods + self.downcastMethods 
                           + self.staticMethods + self.globalMethods):
                if method:
                    if (not (method.typeDescriptor.moduleName in self.CModules)):
                        self.CModules.append(method.typeDescriptor.moduleName)
                        
                        # Now look at all the methods that we might inherit if we are at
                        # a multiple inheritance node and get their C modules
                        if (len(self.parentTypes) >= 2):
                            for parentType in self.parentTypes:
                                for method in parentType.instanceMethods:
                                    if (not (method.typeDescriptor.moduleName in self.CModules)):
                                        self.CModules.append(method.typeDescriptor.moduleName)
                                for method in parentType.upcastMethods:
                                    if (not (method.typeDescriptor.moduleName in self.CModules)):
                                        self.CModules.append(method.typeDescriptor.moduleName)
                                for method in parentType.globalMethods:
                                    if (not (method.typeDescriptor.moduleName in self.CModules)):
                                        self.CModules.append(method.typeDescriptor.moduleName)
                                        
            return self.CModules


    def getReturnTypeModules(self):
        """
        Return a list of all the other shadow class modules this
        class references.
        Be careful about nested types
        """
        moduleList = []

        upcastMethods = []
        if (len(self.parentTypes) >= 2):
            for parentType in self.parentTypes:
                for method in parentType.instanceMethods:
                    upcastMethods.append(method)
                for method in parentType.upcastMethods:
                    upcastMethods.append(method)
                for method in parentType.globalMethods:
                    upcastMethods.append(method)
                    
        for method in (self.constructors + [self.destructor] + self.instanceMethods
                       + self.upcastMethods + self.downcastMethods 
                       + self.staticMethods + self.globalMethods + upcastMethods):
            if method:
                # Get the real return type (not derived)
                returnType = method.typeDescriptor.returnType.recursiveTypeDescriptor()
                if (not returnType.isNested):
                    returnTypeName = returnType.foreignTypeName
                    # Do not put our own module in the import list
                    if ((returnTypeName != self.foreignTypeName) and
                        # Do not put modules already in the list (like a set)
                        (not (returnTypeName in moduleList))):
                        # If this is a class (not a primitive), put it on the list
                        if (returnType.__class__ == ClassTypeDescriptor):
                            moduleList.append(returnTypeName)
                        
                # Now look at all the arguments
                argTypes = method.typeDescriptor.argumentTypes
                for argType in argTypes:
                    # Get the real return type (not derived)
                    argType = argType.typeDescriptor.recursiveTypeDescriptor()
                    if (not argType.isNested):
                        argTypeName = argType.foreignTypeName
                        # Do not put our own module in the import list
                        if ((argTypeName != self.foreignTypeName) and
                            # Do not put modules already in the list (like a set)
                            (not (argTypeName in moduleList))):
                            # If this is a class (not a primitive), put it on the list
                            if (argType.__class__ == ClassTypeDescriptor):
                                moduleList.append(argTypeName)
                   
        return moduleList
    
    def recordClassMethod(self, methodSpec):
        """
        Record all class methods in a 2 level dictionary so we can go
        through them and see which are overloaded
        { className : {methodName : [methodSpec, methodSpec, methodSpec]}}
        """
        methodList = ifAbsentPut(self.overloadedClassMethods, methodSpec.name, [])
        methodList.append(methodSpec)
    
    def recordInstanceMethod(self, methodSpec):
        """
        Record all instance methods in a 2 level dictionary so we can go
        through them and see which are overloaded
        { className : {methodName : [methodSpec, methodSpec, methodSpec]}}
        """
        methodList = ifAbsentPut(self.overloadedInstanceMethods, methodSpec.name, [])
        methodList.append(methodSpec)

    def cullOverloadedMethods(self):
        """
        Find all the entries that have multiple indexes for the same method name
        Get rid of all others.
        """
        tmpDict = {}
        # For each class
        for methodName in self.overloadedClassMethods.keys():
            methodList = self.overloadedClassMethods[methodName]
            # See if this method has more than one function index (overloaded)
            if (len(methodList) > 1):
                tmpDict[methodName] = methodList
                # Mark all the method specifications as overloaded
                for methodSpec in methodList:
                    methodSpec.overloaded = 1
        
        # Now we are done. Jam the tmpDict into the real one
        self.overloadedClassMethods = tmpDict
        
        # Now do the same for instance methods
        tmpDict = {}
        # For each class
        for methodName in self.overloadedInstanceMethods.keys():
            methodList = self.overloadedInstanceMethods[methodName]
            # See if this method has more than one function index (overloaded)
            if (len(methodList) > 1):
                # Copy over the method list
                tmpDict[methodName] = methodList
                # Mark all the method specifications as overloaded
                for methodSpec in methodList:
                    methodSpec.overloaded = 1
        
        # Now we are done. Jam the tmpDict into the real one
        self.overloadedInstanceMethods = tmpDict

    def filterOutStaticMethods(self):
        """
        Run through the list of instance methods and filter out the
        ones that are static class methods. We can tell this because they
        do not have a this pointer in their arg list. Those methods that
        are static are then placed in a new staticMethods list and the ones
        that are left are stored back in the instanceMethods list. We are
        avoiding modifying the instanceMethods list in place while traversing it.
        Do not check upcast or downcast methods because we know they are not static.
        """
        newInstanceMethods = []
        for method in self.instanceMethods:
            if method.isStatic():
                self.staticMethods.append(method)
            else:
                newInstanceMethods.append(method)
        self.instanceMethods = newInstanceMethods

    def recordOverloadedMethods(self):
        """
        Record all the methods in dictionaries based on method name
        so we can see if they are overloaded
        """
        classMethods = self.constructors + self.staticMethods
        if self.destructor:
            classMethods = classMethods + [self.destructor]
        for method in classMethods:
            self.recordClassMethod(method)

        instanceMethods = (self.instanceMethods + self.globalMethods
                           + self.upcastMethods + self.downcastMethods)
        for method in instanceMethods:
            self.recordInstanceMethod(method)

    def generateOverloadedMethods(self, file, nesting):
        """
        Generate code for all the overloaded methods of this class
        """

        if (len(self.overloadedClassMethods.values()) or
            len(self.overloadedInstanceMethods.values())):
            indent(file, nesting+1, '\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '#  Overloaded methods                            #\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '\n')
        
        for methodSpecList in self.overloadedClassMethods.values():
            treeColl = FFIOverload.FFIMethodArgumentTreeCollection(self, methodSpecList)
            treeColl.generateCode(file, nesting)
        for methodSpecList in self.overloadedInstanceMethods.values():
            treeColl = FFIOverload.FFIMethodArgumentTreeCollection(self, methodSpecList)
            treeColl.generateCode(file, nesting)

    def generateGlobalCode(self, dir, extensionsDir):
        """
        Generate shadow class code for this type.
        We make our own file form our foreignTypeName and put it in the dir
        passed in.
        """
        fileName = self.foreignTypeName + '.py'
        file = open(os.path.join(dir, fileName), 'w')
        indent(file, 0, FFIConstants.generatedHeader)
        self.outputBaseImports(file)
        self.generateCode(file, 0)

        # Copy in any extensions we may have
        self.copyExtensions(extensionsDir, file, 0)
        self.outputClassFooter(file)
        file.close()

    def generateCode(self, file, nesting):
        self.recordOverloadedMethods()
        self.cullOverloadedMethods()        
        self.outputImports(file, nesting)
        self.outputClassHeader(file, nesting)
        self.outputClassComment(file, nesting)

        self.outputNestedTypes(file, nesting)

        indent(file, nesting+1, '\n')
        indent(file, nesting+1, '##################################################\n')
        indent(file, nesting+1, '#  Constructors                                  #\n')
        indent(file, nesting+1, '##################################################\n')
        indent(file, nesting+1, '\n')
        self.outputBaseConstructor(file, nesting)
        if self.constructors:
            for method in self.constructors:
                method.generateConstructorCode(self, file, nesting)
        else:
            self.outputEmptyConstructor(file, nesting)

        indent(file, nesting+1, '\n')
        indent(file, nesting+1, '##################################################\n')
        indent(file, nesting+1, '#  Destructor                                    #\n')
        indent(file, nesting+1, '##################################################\n')
        indent(file, nesting+1, '\n')
        self.outputBaseDestructor(file, nesting)
        if self.destructor:
            self.destructor.generateDestructorCode(self, file, nesting)
        else:
            self.outputEmptyDestructor(file, nesting)

        if len(self.staticMethods):
            indent(file, nesting+1, '\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '#  Static Methods                                #\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '\n')
            for method in self.staticMethods:
                method.generateStaticCode(self, file, nesting)

        if len(self.instanceMethods):
            indent(file, nesting+1, '\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '#  Instance methods                              #\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '\n')
            for method in self.instanceMethods:
                method.generateMethodCode(self, file, nesting)

        if len(self.upcastMethods):
            indent(file, nesting+1, '\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '#  Upcast methods                                #\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '\n')
            for method in self.upcastMethods:
                method.generateUpcastMethodCode(self, file, nesting)

        if len(self.downcastMethods):
            indent(file, nesting+1, '\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '#  Downcast methods                              #\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '\n')
            for method in self.downcastMethods:
                method.generateDowncastMethodCode(self, file, nesting)

        if len(self.globalMethods):
            indent(file, nesting+1, '\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '#  Global methods                                #\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '\n')
            for method in self.globalMethods:
                method.generateMethodCode(self, file, nesting)

        # At multiple inheritance nodes, copy all the parent methods into
        # this class and call them after upcasting us to that class
        if (len(self.parentTypes) >= 2):
            indent(file, nesting+1, '\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '#  Upcast inherited instance method wrappers     #\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '\n')
            for parentType in self.parentTypes:
                # Copy all the parents instance methods
                for method in parentType.instanceMethods:
                    method.generateInheritedUpcastMethodCode(self, parentType, file, nesting)
                # Copy all the parents upcast methods so we transitively pick them up
                for method in parentType.upcastMethods:
                    method.generateInheritedUpcastMethodCode(self, parentType, file, nesting)
                # Do not copy the downcast methods

        # At multiple inheritance nodes, copy all the parent methods into
        # this class and call them after upcasting us to that class
        if (len(self.parentTypes) >= 2):
            indent(file, nesting+1, '\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '#  Upcast global method wrappers                 #\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '\n')
            for parentType in self.parentTypes:
                for method in parentType.globalMethods:
                    method.generateInheritedUpcastMethodCode(self, parentType, file, nesting)

        self.generateOverloadedMethods(file, nesting)

    def outputNestedTypes(self, file, nesting):
        if (len(self.nestedTypes) > 0):
            indent(file, nesting+1, '\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '#  Nested Types                                  #\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '\n')
        # Output code in this same file for all our nested types
        for nestedType in self.nestedTypes:
            nestedType.generateCode(file, nesting+1)

    def copyExtensions(self, extensionsDir, file, nesting):
        """
        Copy in the extension file for this class if one exists
        If you want to extend a C++ file, create a file in the extensions directory and
        this will append that extension file to the generated code file.
        """
        extensionFileName = self.getExtensionModuleName()
        extensionFilePath = os.path.join(extensionsDir, extensionFileName)
        if os.path.exists(extensionFilePath):
            FFIConstants.notify.info( 'Found extensions for class: ' + self.foreignTypeName)
            extensionFile = open(extensionFilePath)
            indent(file, nesting+1, '\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '#  Extension methods                             #\n')
            indent(file, nesting+1, '##################################################\n')
            indent(file, nesting+1, '\n')
            # Copy the contents of the extensions file to the class file verbatim
            indent(file, nesting, extensionFile.read())
        else:
            # No extensions for this class
            pass
        

    def outputBaseImports(self, file):
        indent(file, 0, 'import ' + FFIConstants.staticModuleName + '\n')
        # Everybody imports types for type checking
        indent(file, 0, 'import types\n')
        indent(file, 0, '\n')

        indent(file, 0, '# Import all the C modules this class uses\n')
        for moduleName in self.getCModules():
            indent(file, 0, 'import ' + moduleName + '\n')            
            indent(file, 0, 'import ' + moduleName + 'Downcasts\n')
        indent(file, 0, '\n')
        indent(file, 0, 'import FFIExternalObject\n')
        
        
    def outputImports(self, file, nesting):
        """
        Generate code that imports the modules we need for this class
        """

        if len(self.parentTypes):
            indent(file, nesting, '# Import everybody we inherit from\n')
            for parent in self.parentTypes:
                indent(file, nesting, 'import ' + parent.foreignTypeName + '\n')
            indent(file, nesting, '\n')

        returnTypeModules = self.getReturnTypeModules()
        if len(returnTypeModules):
            indent(file, nesting, '# Import all the shadow class modules this class uses\n')
            for moduleName in returnTypeModules:
                indent(file, nesting, 'import ' + moduleName + '\n')
            
        # an extra line just for spacing
        indent(file, nesting, '\n')

    def outputClassComment(self, file, nesting):
        """
        Output the class comment to the file
        """
        if FFIConstants.wantComments:
            if self.comment:
                indent(file, nesting+1, ('"' * 3) + '\n')
                # To insert tabs into the comment, replace all newlines with a newline+tabs
                comment = string.replace(self.comment,
                                         '\n', ('\n' + ('    ' * (nesting+1))))
                indent(file, nesting+1, comment)
                file.write('\n')
                indent(file, nesting+1, ('"' * 3) + '\n\n')
    
    def outputClassHeader(self, file, nesting):
        """
        Output the class definition to the file
        """

        if (self.foreignTypeName == ''):
            FFIConstants.notify.warning('Class with no name')

        # If this is the toplevel, we need to delay the generation of this
        # class to avoid circular imports, so put the entire class in a function
        # that we will call later
        if (nesting==0):
            indent(file, nesting, '# Delay the definition of this class until all the imports are done\n')
            indent(file, nesting, '# Make sure we only define this class once\n')
            indent(file, nesting, 'classDefined = 0\n')
            indent(file, nesting, 'def generateClass_' + self.foreignTypeName + '():\n')
            indent(file, nesting, ' if classDefined: return\n')
            indent(file, nesting, ' global classDefined\n')
            indent(file, nesting, ' classDefined = 1\n')
            # Start the class definition indented a space to account for the function
            indent(file, nesting, ' class ' + self.foreignTypeName)
        else:
            # Start the class definition
            indent(file, nesting, 'class ' + self.foreignTypeName)

        # Everybody inherits from FFIExternalObject
        file.write('(')
        # Also inherit from all of your parentTypes
        for i in range(len(self.parentTypes)):
            parentTypeName = self.parentTypes[i].foreignTypeName
            # assuming the type "Node" is stored in module "Node.py"
            # and we have done an "import Node", we need to then
            # inherit from Node.Node
            file.write(parentTypeName + '.' + parentTypeName)
            file.write(', ')
        file.write('FFIExternalObject.FFIExternalObject):\n')

        # Store the class C modules for the class so they do not
        # get garbage collected before we do
        # TODO: this did not appear to work
        indent(file, nesting+1, '__CModules__ = [')
        for moduleName in self.getCModules():
            file.write(moduleName + ',')
        file.write(']\n')

        # Store the downcast function modules so the FFIExternalObject
        # can index into them to find the downcast functions
        indent(file, nesting+1, '__CModuleDowncasts__ = [')
        for moduleName in self.getCModules():
            file.write(moduleName + 'Downcasts,')
        file.write(']\n')


    def outputClassFooter(self, file):
        indent(file, 0, " # When this class gets defined, put it in this module's namespace\n")
        indent(file, 0, " globals()['" + self.foreignTypeName + "'] = " + self.foreignTypeName + '\n')
    
    def outputBaseConstructor(self, file, nesting):
        """
        Output the __init__ constructor for this class.
        There is special login if you pass in None to the constructor, you
        will not get an actual C object with memory, you will just get the
        shadow class shell object. This is useful for functions that want
        to return this type that already have a this pointer and just need
        to construct a shadow object to contain it.
        """

        indent(file, nesting+1, 'def __init__(self, *_args):\n')
        indent(file, nesting+2, '# Initialize the super class\n')
        indent(file, nesting+2, 'FFIExternalObject.FFIExternalObject.__init__(self)\n')
        indent(file, nesting+2, '# If you want an empty shadow object, pass in None\n')
        indent(file, nesting+2, 'if ((len(_args) == 1) and (_args[0] == None)):\n')
        indent(file, nesting+3, 'return\n')
        indent(file, nesting+2, '# Otherwise, call the C constructor\n')
        indent(file, nesting+2, 'apply(self.constructor, _args)\n')
        indent(file, nesting+2, '\n')

    def outputEmptyConstructor(self, file, nesting):
        """
        If there is no C++ constructor, we output code for a runtime error
        You really do not want to create a class with a null this pointer
        """
        indent(file, nesting+1, 'def constructor(self):\n')
        indent(file, nesting+2, "raise RuntimeError, 'No C++ constructor defined for class: ' + self.__class__.__name__\n")

    def outputBaseDestructor(self, file, nesting):
        """
        This destructor overwrites the builtin Python destructor
        using the __del__ method. This will get called whenever a
        Python object is garbage collected. We are going to overwrite
        it with special cleanup for Panda.
        """
        indent(file, nesting+1, 'def __del__(self):\n')

        # Reference counting is now handled in the C++ code
        # indent(file, nesting+2, 'if isinstance(self, ReferenceCount):\n')
        # indent(file, nesting+3, 'self.unref()\n')
        # indent(file, nesting+3, 'if (self.getCount() == 0):\n')
        # indent(file, nesting+4, 'self.destructor()\n')

        # If the scripting language owns the memory for this object,
        # we need to call the C++ destructor when Python frees the
        # shadow object, but only if the userManagesMemory flag is set.
        # Also make sure we are not destructing a null pointer
        indent(file, nesting+2, 'if (self.userManagesMemory and (self.this != 0)):\n')
        indent(file, nesting+3, 'self.destructor()\n')

    def outputEmptyDestructor(self, file, nesting):
        """
        If there is no C++ destructor, we just output this
        empty one instead
        """
        indent(file, nesting+1, 'def destructor(self):\n')
        indent(file, nesting+2, 'pass\n')

    def generateReturnValueWrapper(self, file, userManagesMemory,
                                   needsDowncast, nesting):
        """
        Generate code that creates a shadow object of this type
        then sets the this pointer and returns the object. We call the
        class destructor with None as the only parameter to get an
        empty shadow object.
        """
        indent(file, nesting, 'returnObject = ')
        # Do not put Class.Class if this file is the file that defines Class
        if (os.path.basename(file.name)[:-3] == self.foreignTypeName):
            file.write(self.foreignTypeName)
        else:
            file.write(self.foreignTypeName + '.' + self.foreignTypeName)
        file.write('(None)\n')
        indent(file, nesting, 'returnObject.this = returnValue\n')
        if userManagesMemory:
            indent(file, nesting, 'returnObject.userManagesMemory = 1\n')
        if needsDowncast:
            indent(file, nesting, 'return returnObject.setPointer()\n')
        else:
            indent(file, nesting, 'return returnObject\n')
            


class FunctionTypeDescriptor(BaseTypeDescriptor):
    """
    A C++ function type. It knows its returnType, arguments, etc.
    """
    def __init__(self):
        BaseTypeDescriptor.__init__(self)
        self.returnType = None
        self.argumentTypes = []
        self.userManagesMemory = 0
        self.isVirtual = 0
        self.moduleName = ''
        self.wrapperName = ''
        self.returnValueDestructor = None
    def thislessArgTypes(self):
        """
        It is often useful to know the list of arguments excluding the
        this parameter (if there was one)
        """
        return filter(lambda type: (not type.isThis), self.argumentTypes)

