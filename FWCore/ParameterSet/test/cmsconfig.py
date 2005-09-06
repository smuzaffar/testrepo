#------------------------------------------------------------
#
# $Id:$
#
# cmsconfig: a class to provide convenient access to the Python form
# of a parsed CMS configuration file.
#
# Note: we have not worried about security. Be careful about strings
# you put into this; we use a naked 'eval'!
#
#------------------------------------------------------------
#
# Tests for this class need to be run 'by hand', until we figure out
# how to make use of the SCRAMV1 tools for running tests on Python
# code.
#
#------------------------------------------------------------
# TODO: We need some refactoring to handle the writing of module-like
# objects more gracefully. Right now, we have to pull the classname
# out of the dictionary in more than one place. Try making a class
# which represents the module, which contains the dictionary now used,
# and knows about special features: the class name, now to print the
# guts without repeating the classname, etc.
#------------------------------------------------------------

import cStringIO
import types

# TODO: Refactor pset_dict_to_string and class printable_parameter to
# have a consistent view of the problem. Perhaps have a class
# representing the configuration data for a PSet object, rather than
# just using a dictionary instance. See also __write_module_guts,
# which should be refactored at the same time.

def pset_dict_to_string(psetDict):
    """Convert dictionary representing a PSet to a string consistent
    with the configuration grammar."""
    stream = cStringIO.StringIO()
    stream.write('\n{\n')

    for name, value in psetDict.iteritems():
        stream.write('%s' % printable_parameter(name, value))
        stream.write('\n')        
    
    stream.write('}\n')
    return stream.getvalue()

class printable_parameter:
    """A class to provide automatic unpacking of the tuple (triplet)
    representation of a single parameter, suitable for printing.

    Note that 'value' may in fact be a list."""
    
    def __init__(self, aName, aValueTuple):
        self.name = aName
        self.type, self.trackedCode, self.value = aValueTuple
        # Because the configuration grammar treats tracked status as
        # the default, we only have to write 'untracked' as the
        # tracking code if the parameter is untracked.        
        if self.trackedCode == "tracked":
            self.trackedCode = ""
        else:
            self.trackedCode = "untracked " # trailing space is needed

        # We need special handling of some of the parameter types.
        if self.type in ["vbool", "vint32", "vuint32", "vdouble", "vstring"]:
            # TODO: Consider using cStringIO, if this is observed
            # to be a bottleneck. This may happen if many large
            # vectors are used in parameter sets.
            temp = '{'
            # Write out values as a comma-separated list
            temp += ", ".join(self.value)
            temp += '}'
            self.value = temp

        if self.type == "PSet":
            self.value = pset_dict_to_string(self.value)

    def __str__(self):
        """Print this parameter in the right format for a
        configuration file."""
        s = "%(trackedCode)s%(type)s %(name)s = %(value)s" % self.__dict__
        return s    

# I'm not using new-style classes, because I'm not sure that we can
# rely on a new enough version of Python to support their use.

class cmsconfig:
    """A class to provide convenient access to the contents of a
    parsed CMS configuration file."""
    
    def __init__(self, stringrep):
        """Create a cmsconfig object from the contents of the (Python)
        exchange format for configuration files."""
        self.psdata = eval(stringrep)

    def numberOfModules(self):
        return len(self.psdata['modules'])

    def numberOfOutputModules(self):
        return len(self.getOutputModuleNames())

    def moduleNames(self):
        return self.psdata['modules'].keys()

    def module(self, name):
        """Get the module with this name. Exception raised if name is
        not known. Returns a dictionary."""
        return self.psdata['modules'][name]

    def outputModuleNames(self):
        return self.psdata['output_modules']

    def pathNames(self):
        return self.psdata['paths'].keys()

    def path(self, name):
        """Get the path description for the path of the given
        name. Exception raised if name is not known. Returns a
        string."""
        return self.psdata['paths'][name]

    def sequenceNames(self):
        return self.psdata['sequences'].keys()

    def sequence(self, name):
        """Get the sequence description for the sequence of the given
        name. Exception raised if name is not known. Returns a
        string."""
        return self.psdata['sequences'][name]

    def endpath(self):
        """Return the endpath description, as a string."""
        return self.psdata['endpath']

    def mainInputSource(self):
        """Return the description of the main input source, as a
        dictionary."""
        return self.psdata['main_input']

    def procName(self):
        """Return the process name, a string"""
        return self.psdata['procname']

    def asConfigurationString(self):
        """Return a string conforming to the configuration file
        grammar, encoding this configuration."""

        # Let's try to make sure we lose no resources if something
        # fails in formatting...
        result = ""
        
        try:
            stream = cStringIO.StringIO()
            self.__write_self_to_stream(stream)
            result = stream.getvalue()

        finally:
            stream.close()
        
        return result

    def __write_self_to_stream(self, fileobj):
        """Private method.
        Return None.
        Write the contents of self to the file-like object fileobj."""

        # Write out the process block
        fileobj.write('process %s = \n{\n' % self.procName())
        self.__write_process_block_guts(fileobj)
        fileobj.write('}\n')

    def __write_process_block_guts(self, fileobj):
        """Private method.
        Return None.
        Write the guts of the process block to the file-like object
        fileobj."""

        # TODO: introduce, and deal with, top-level PSet objects and
        # top-level block objects.        
        self.__write_main_source(fileobj)
        self.__write_modules(fileobj)
        self.__write_sequences(fileobj)
        self.__write_paths(fileobj)
        self.__write_endpath(fileobj)

    def __write_modules(self, fileobj):
        """Private method.
        Return None
        Write all the modules to the file-like object fileobj."""
        for name in self.moduleNames():
            moddict = self.module(name)
            fileobj.write("module %s = %s\n{\n" % (name, moddict['classname'][2]))
            self.__write_module_guts(moddict, fileobj)
            fileobj.write('}\n')


    def __write_sequences(self, fileobj):
        """Private method.
        Return None
        Write all the sequences to the file-like object fileobj."""
        for name in self.sequenceNames():
            fileobj.write("sequence %s = {%s}\n"  % (name, self.sequence(name)))


    def __write_paths(self, fileobj):
        """Private method.
        Return None
        Write all the paths to the file-like object fileobj."""
        for name in self.pathNames():
            fileobj.write("path %s = {%s}\n" % (name, self.path(name)))

        
    def __write_endpath(self, fileobj):
        """Private method.
        Return None
        Write the endpath statement to the file-like object
        fileobj."""
        fileobj.write( "endpath = {%s}\n" % self.endpath())

    def __write_main_source(self, fileobj):
        """Private method.
        Return None
        Write the (main) source block to the file-like object
        fileobj."""
        mis = self.mainInputSource()  # this is a dictionary
        fileobj.write('source = %s\n{\n' % mis['classname'][2])
        self.__write_module_guts(mis, fileobj)
        fileobj.write('}\n')

    
    def __write_module_guts(self, moddict, fileobj):
        """Private method.
        Return None
        Print the body of the block for this 'module'. This includes
        all the dictionary contents except for the classname (because
        the classname does not appear within the block).

        NOTE: This should probably be a static method, because it doesn't
        use any member data of the object, but I'm not sure we can
        rely on a new-enough version of Python to make use of static
        methods."""
        for name, value in moddict.iteritems():
            if name != 'classname':
                fileobj.write('%s' % printable_parameter(name, value))
                fileobj.write('\n')

            


    
        

        
