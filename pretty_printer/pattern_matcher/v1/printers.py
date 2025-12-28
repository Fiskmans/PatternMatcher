

import re
import gdb
import sys
from builtins import chr

if sys.version_info[0] > 2:
    # Python 3 stuff
    Iterator = object
    # Python 3 folds these into the normal functions.
    imap = map
    izip = zip
    # Also, int subsumes long
    long = int
else:
    # Python 2 stuff
    class Iterator:
        """Compatibility mixin for iterators

        Instead of writing next() methods for iterators, write
        __next__() methods and use this mixin to make them work in
        Python 2 as well as Python 3.

        Idea stolen from the "six" documentation:
        <http://pythonhosted.org/six/#six.Iterator>
        """

        def next(self):
            return self.__next__()

    # In Python 2, we still need these from itertools
    from itertools import imap, izip

# Try to use the new-style pretty-printing if available.
_use_gdb_pp = True
try:
    import gdb.printing
except ImportError:
    _use_gdb_pp = False

# Try to install type-printers.
_use_type_printing = False
try:
    import gdb.types
    if hasattr(gdb.types, 'TypePrinter'):
        _use_type_printing = True
except ImportError:
    pass


# Starting with the type ORIG, search for the member type NAME.  This
# handles searching upward through superclasses.  This is needed to
# work around http://sourceware.org/bugzilla/show_bug.cgi?id=13615.
def find_type(orig, name):
    typ = orig.strip_typedefs()
    while True:
        search = str(typ) + '::' + name
        try:
            return gdb.lookup_type(search)
        except RuntimeError:
            pass
        # The type was not found, so try the superclass.  We only need
        # to check the first superclass, so we don't bother with
        # anything fancier here.
        field = typ.fields()[0]
        if not field.is_base_class:
            raise ValueError("Cannot find type %s::%s" % (orig, name))
        typ = field.type


void_type = gdb.lookup_type('void')


def ptr_to_void_ptr(val):
    if gdb.types.get_basic_type(val.type).code == gdb.TYPE_CODE_PTR:
        return val.cast(void_type.pointer())
    else:
        return val


class FragmentPrinter:
    "pattern_matcher::Fragment"

    def __init__(self, typename, val):
        self.val = val
        self.typename = typename
        self.enabled = True
    
    def children(self):
        return []

    def to_string(self):
        # Make sure & works, too.
        type = self.val.type
        if type.code == gdb.TYPE_CODE_REF:
            type = type.target()

        fragType = str(self.val['myType']);

        match fragType:
            case 'pattern_matcher::Fragment::Type::Literal':
                return  "<" + str(self.val["myLiteral"].bytes) + ">";

            case 'pattern_matcher::Fragment::Type::Sequence':
                return  "[" + str(self.val["mySubFragments"]) + "]";

            case 'pattern_matcher::Fragment::Type::Alternative':
                return  "(" + str(self.val["mySubFragments"]) + ")";

            case _:
                return 'Unkown Type: ' + fragType

    def display_hint(self):
        return 'string'


class Printer(object):
    def __init__(self, name):
        super(Printer, self).__init__()
        self.name = name
        self.subprinters = []
        self.lookup = []
        self.enabled = True

    def add(self, name, function):
        print("[patternmatcher] Adding " + name)
        self.lookup.append((name, function))

    @staticmethod
    def get_basic_type(type):
        # If it points to a reference, get the reference.
        if type.code == gdb.TYPE_CODE_REF:
            type = type.target()

        # Get the unqualified type, stripped of typedefs.
        type = type.unqualified().strip_typedefs()

        return type.tag

    def __call__(self, val):
        typename = self.get_basic_type(val.type)
        if not typename:
            return None

        for (name, printer) in self.lookup:
            if name == typename:
                return printer(name, val)

        # Cannot find a pretty printer.  Return None.
        return None


MainPrinter = None

def register_type_printers(obj):
    global _use_type_printing

    if not _use_type_printing:
        return
    print("[patternmatcher] Registering type printers")

    gdb.types.register_type_printer(obj, TypePrinter("pattern_matcher::Fragment"));



def register_pattern_matcher_printers(obj):
    "Register pattern_matcher pretty-printers with objfile Obj."

    global _use_gdb_pp
    global MainPrinter

    print("[patternmatcher] Registering")

    gdb.pretty_printers.append(MainPrinter)


def build_pattern_matcher_dictionary():
    global MainPrinter

    print("[patternmatcher] Initializing")

    MainPrinter = Printer("pattern_matcher")

    MainPrinter.add('pattern_matcher::Fragment', FragmentPrinter)

build_pattern_matcher_dictionary()
