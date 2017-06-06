
from Mixins import _ConfigureComponent
from Mixins import _Labelable, _Unlabelable
from Mixins import _ValidatingParameterListBase
from ExceptionHandling import *

class _Sequenceable(object):
    """Denotes an object which can be placed in a sequence"""
    def __mul__(self,rhs):
        return _SequenceOpAids(self,rhs)
    def __add__(self,rhs):
        return _SequenceOpFollows(self,rhs)
    def __invert__(self):
        return _SequenceNegation(self)
    def _clonesequence(self, lookuptable):
        try: 
            return lookuptable[id(self)]
        except:
            raise KeyError("no "+str(type(self))+" with id "+str(id(self))+" found")
    def isOperation(self):
        """Returns True if the object is an operator (e.g. *,+ or !) type"""
        return False
    def _visitSubNodes(self,visitor):
        pass
    def visitNode(self,visitor):
        visitor.enter(self)
        self._visitSubNodes(visitor)
        visitor.leave(self)
class _ModuleSequenceType(_ConfigureComponent, _Labelable):
    """Base class for classes which define a sequence of modules"""
    def __init__(self,*arg, **argv):
        if len(arg) != 1:
            typename = format_typename(self)
            msg = format_outerframe(2) 
            msg += "%s takes exactly one input value. But the following ones are given:\n" %typename
            for item,i in zip(arg, xrange(1,20)):
                msg += "    %i) %s \n"  %(i, item._errorstr())
            msg += "Maybe you forgot to combine them via '*' or '+'."     
            raise TypeError(msg)
        self._checkIfSequenceable(arg[0])
        self._seq = arg[0]
        self._isModified = False
    def _place(self,name,proc):
        self._placeImpl(name,proc)
    def __imul__(self,rhs):
        self._checkIfSequenceable(rhs)
        self._seq = _SequenceOpAids(self._seq,rhs)
        return self
    def __iadd__(self,rhs):
        self._checkIfSequenceable(rhs)
        self._seq = _SequenceOpFollows(self._seq,rhs)
        return self
    def _checkIfSequenceable(self,v):
        if not isinstance(v,_Sequenceable):
            typename = format_typename(self)
            msg = format_outerframe(2)
            msg += "%s only takes arguments of types which are allowed in a sequence, but was given:\n" %typename
            msg +=format_typename(v)
            msg +="\nPlease remove the problematic object from the argument list"
            raise TypeError(msg)
    def __str__(self):
        return str(self._seq)
    def dumpConfig(self, options):
        return '{'+self._seq.dumpSequenceConfig()+'}\n'
    def dumpPython(self, options):
        return 'cms.'+type(self).__name__+'('+self._seq.dumpSequencePython()+')\n'
    def __repr__(self):
        return "cms."+type(self).__name__+'('+str(self._seq)+')\n'
    def copy(self):
        returnValue =_ModuleSequenceType.__new__(type(self))
        returnValue.__init__(self._seq)
        return returnValue
    def _postProcessFixup(self,lookuptable):
        self._seq = self._seq._clonesequence(lookuptable)
        return self
    #def replace(self,old,new):
    #"""Find all instances of old and replace with new"""
    #def insertAfter(self,which,new):
    #"""new will depend on which but nothing after which will depend on new"""
    #((a*b)*c)  >> insertAfter(b,N) >> ((a*b)*(N+c))
    #def insertBefore(self,which,new):
    #"""new will be independent of which"""
    #((a*b)*c) >> insertBefore(b,N) >> ((a*(N+b))*c)
    #def __contains__(self,item):
    #"""returns whether or not 'item' is in the sequence"""
    #def modules_(self):
    def _findDependencies(self,knownDeps,presentDeps):
        self._seq._findDependencies(knownDeps,presentDeps)
    def moduleDependencies(self):
        deps = dict()
        self._findDependencies(deps,set())
        return deps
    def nameInProcessDesc_(self, myname):
        return myname
    def fillNamesList(self, l):
        return self._seq.fillNamesList(l)
    def insertInto(self, parameterSet, myname):
        # represented just as a list of names in the ParameterSet
        l = []
        self.fillNamesList(l)
        parameterSet.addVString(True, myname, l)
    def visit(self,visitor):
        """Passes to visitor's 'enter' and 'leave' method each item describing the module sequence.
        If the item contains 'sub' items then visitor will see those 'sub' items between the
        item's 'enter' and 'leave' calls.
        """
        self._seq.visitNode(visitor)

class _SequenceOpAids(_Sequenceable):
    """Used in the expression tree for a sequence as a stand in for the ',' operator"""
    def __init__(self, left, right):
        self.__left = left
        self.__right = right
    def __str__(self):
        return str(self.__left)+'*'+str(self.__right)
    def dumpSequenceConfig(self):
        return '('+self.__left.dumpSequenceConfig()+','+self.__right.dumpSequenceConfig()+')'
    def dumpSequencePython(self):
        return self.__left.dumpSequencePython()+'*'+self.__right.dumpSequencePython()
    def _findDependencies(self,knownDeps,presentDeps):
        #do left first and then right since right depends on left
        self.__left._findDependencies(knownDeps,presentDeps)
        self.__right._findDependencies(knownDeps,presentDeps)
    def _clonesequence(self, lookuptable):
        return type(self)(self.__left._clonesequence(lookuptable),self.__right._clonesequence(lookuptable))
    def fillNamesList(self, l):
        self.__left.fillNamesList(l)
        self.__right.fillNamesList(l)
    def isOperation(self):
        return True
    def _visitSubNodes(self,visitor):
        self.__left.visitNode(visitor)        
        self.__right.visitNode(visitor)

class _SequenceNegation(_Sequenceable):
    """Used in the expression tree for a sequence as a stand in for the '!' operator"""
    def __init__(self, operand):
        self.__operand = operand
    def __str__(self):
        return '~%s' %self.__operand
    def dumpSequenceConfig(self):
        return '!%s' %self.__operand.dumpSequenceConfig()
    def dumpSequencePython(self):
        return '~%s' %self.__operand.dumpSequencePython()
    def _findDependencies(self,knownDeps, presentDeps):
        self.__operand._findDependencies(knownDeps, presentDeps)
    def fillNamesList(self, l):
        l.append(self.__str__())
    def _clonesequence(self, lookuptable):
        return type(self)(self.__operand._clonesequence(lookuptable))
    def isOperation(self):
        return True
    def _visitSubNodes(self,visitor):
        self.__operand.visitNode(visitor)


class _SequenceOpFollows(_Sequenceable):
    """Used in the expression tree for a sequence as a stand in for the '&' operator"""
    def __init__(self, left, right):
        self.__left = left
        self.__right = right
    def __str__(self):
        return str(self.__left)+'+'+str(self.__right)
    def dumpSequenceConfig(self):
        return '('+self.__left.dumpSequenceConfig()+'&'+self.__right.dumpSequenceConfig()+')'
    def dumpSequencePython(self):
        return self.__left.dumpSequencePython()+'+'+self.__right.dumpSequencePython()
    def _findDependencies(self,knownDeps,presentDeps):
        oldDepsL = presentDeps.copy()
        oldDepsR = presentDeps.copy()
        self.__left._findDependencies(knownDeps,oldDepsL)
        self.__right._findDependencies(knownDeps,oldDepsR)
        end = len(presentDeps)
        presentDeps.update(oldDepsL)
        presentDeps.update(oldDepsR)
    def _clonesequence(self, lookuptable):
        return type(self)(self.__left._clonesequence(lookuptable),self.__right._clonesequence(lookuptable))
    def fillNamesList(self, l):
        self.__left.fillNamesList(l)
        self.__right.fillNamesList(l)
    def isOperation(self):
        return True
    def _visitSubNodes(self,visitor):
        self.__left.visitNode(visitor)        
        self.__right.visitNode(visitor)



class Path(_ModuleSequenceType):
    def __init__(self,*arg,**argv):
        super(Path,self).__init__(*arg,**argv)
    def _placeImpl(self,name,proc):
        proc._placePath(name,self)


class EndPath(_ModuleSequenceType):
    def __init__(self,*arg,**argv):
        super(EndPath,self).__init__(*arg,**argv)
    def _placeImpl(self,name,proc):
        proc._placeEndPath(name,self)


class Sequence(_ModuleSequenceType,_Sequenceable):
    def __init__(self,*arg,**argv):
        super(Sequence,self).__init__(*arg,**argv)
    def _placeImpl(self,name,proc):
        proc._placeSequence(name,self)
    def _clonesequence(self, lookuptable):
        if id(self) not in lookuptable:
            #for sequences held by sequences we need to clone
            # on the first reference
            clone = type(self)(self._seq._clonesequence(lookuptable))
            lookuptable[id(self)]=clone
            lookuptable[id(clone)]=clone
        return lookuptable[id(self)]
    def _visitSubNodes(self,visitor):
        self.visit(visitor)

class SequencePlaceholder(_ModuleSequenceType,_Sequenceable):
    def __init__(self, name):
        self._name = name
    def _placeImpl(self,name,proc):
        pass
    def insertInto(self, parameterSet, myname):
        raise RuntimeError("The SequencePlaceholder "+self._name
                           +" was never overridden")
    def copy(self):
        returnValue =SequencePlaceholder.__new__(type(self))
        returnValue.__init__(self._name)
        return returnValue
    def dumpPython(self, options):
        return 'cms.SequencePlaceholder(\"'+self._name+'\")\n'

    

class Schedule(_ValidatingParameterListBase,_ConfigureComponent,_Unlabelable):
    def __init__(self,*arg,**argv):
        super(Schedule,self).__init__(*arg,**argv)
    @staticmethod
    def _itemIsValid(item):
        return isinstance(item,Path) or isinstance(item,EndPath)
    def copy(self):
        import copy
        return copy.copy(self)
    def _place(self,label,process):
        process.setSchedule_(self)
    def fillNamesList(self, l):
        for seq in self:
            seq.fillNamesList(l)

if __name__=="__main__":
    import unittest
    class DummyModule(_Sequenceable):
        def __init__(self,name):
            self._name = name
        def __str__(self):
            return self._name
        def dumpSequenceConfig(self):
            return self._name
        def dumpSequencePython(self):
            return 'process.'+self._name
    class TestModuleCommand(unittest.TestCase):
        def setUp(self):
            """Nothing to do """
            print 'testing'
        def testDumpPython(self):
            a = DummyModule("a")
            b = DummyModule('b')
            p = Path((a*b))
            self.assertEqual(p.dumpPython(None),"cms.Path(process.a*process.b)\n")
            p2 = Path((b+a))
            self.assertEqual(p2.dumpPython(None),"cms.Path(process.b+process.a)\n")
            c = DummyModule('c')
            p3 = Path(c*(a+b))
            self.assertEqual(p3.dumpPython(None),"cms.Path(process.c*process.a+process.b)\n")
        def testVisitor(self):
            class TestVisitor(object):
                def __init__(self, enters, leaves):
                    self._enters = enters
                    self._leaves = leaves
                def enter(self,visitee):
                    #print visitee
                    if self._enters[0] != visitee:
                        raise RuntimeError("wrong node ("+str(visitee)+") on 'enter'")
                    else:
                        self._enters = self._enters[1:]
                def leave(self,visitee):
                    if self._leaves[0] != visitee:
                        raise RuntimeError("wrong node ("+str(visitee)+") on 'leave'\n expected ("+str(self._leaves[0])+")")
                    else:
                        self._leaves = self._leaves[1:]
            a = DummyModule("a")
            b = DummyModule('b')
            multAB = a*b
            p = Path(multAB)
            t = TestVisitor(enters=[multAB,a,b],
                            leaves=[a,b,multAB])
            p.visit(t)

            plusAB = a+b
            p = Path(plusAB)
            t = TestVisitor(enters=[plusAB,a,b],
                            leaves=[a,b,plusAB])
            p.visit(t)
            
            s=Sequence(plusAB)
            c=DummyModule("c")
            multSC = s*c
            p=Path(multSC)
            t=TestVisitor(enters=[multSC,s,plusAB,a,b,c],
                          leaves=[a,b,plusAB,s,c,multSC])
            p.visit(t)
            
            notA= ~a
            p=Path(notA)
            t=TestVisitor(enters=[notA,a],leaves=[a,notA])
            p.visit(t)
    
    unittest.main()

        
