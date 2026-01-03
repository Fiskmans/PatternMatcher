

import re
import gdb
import sys
import string
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

def pairwise(iterable):
    "s -> (s0, s1), (s2, s3), (s4, s5), ..."
    a = iter(iterable)
    return zip(a, a)

def Char(chars):

    c = chars[0]
    match c:
        case 9:
            return '\\t'
        case 10:
            return '\\n'
        case 11:
            return '\\v'
        case 12:
            return format(chars[0], '02x')
        case 13:
            return '\\r'
        case 32:
            return '\' \''
        case 124:
            return '\'|\''


    if chars[0] in bytes(string.printable, "ascii"):
        return chars.decode("ascii")

    return format(chars[0], '02x')

NameLookup = {}

def NameOf(fragment):
    global NameLookup
    if str(fragment.dereference()["myType"]) == "pattern_matcher::Fragment::Type::Literal":
        return Char(fragment.dereference()["myLiteral"].bytes)
    
    addr = str(fragment)

    if addr in NameLookup:
        return NameLookup[addr]

    return str(fragment)


def ExtractSubFragments(arr):
    out = []

    start = arr["_M_impl"]["_M_start"]
    end = arr["_M_impl"]["_M_finish"]
    length = end - start

    for i in range(length):
        out.append(NameOf(start[i]))

    return out

def AddToNameLookup(m):
    global NameLookup
    
    visualizer = gdb.default_visualizer(m)

    if not "children" in dir(visualizer):
        return

    for ((keyname, key), (valuename, value)) in pairwise(visualizer.children()):
        NameLookup[str(value.address)] = str(key).strip('\"')
        

class PatternMatcherPrinter(gdb.ValuePrinter):
    "pattern_matcher::PatternMatcher"

    def __init__(self, typename, val):
        self.val = val
        self.typename = typename
        self.enabled = True

        AddToNameLookup(val["myFragments"])

    def children(self):
        return [ ("fragments", "fragments"),("fragments", self.val["myFragments"]), ("literals", "literals"), ("literals", self.val["myLiterals"])]

    def to_string(self):
        return "KeyType=" + self.val.type.template_argument(0).name + " with " + str(self.val["myFragments"]["_M_h"]["_M_element_count"]) + " fragments"

    def display_hint(self):
        return "map"

class FragmentPrinter:
    "pattern_matcher::Fragment"

    def __init__(self, typename, val):
        self.val = val
        self.typename = typename
        self.enabled = True

    def to_string(self):
        # Make sure & works, too.
        type = self.val.type
        if type.code == gdb.TYPE_CODE_REF:
            type = type.target()

        fragType = str(self.val['myType']);

        match fragType:
            case 'pattern_matcher::Fragment::Type::Literal':
                return Char(self.val["myLiteral"].bytes)

            case 'pattern_matcher::Fragment::Type::Sequence':
                return " ".join(ExtractSubFragments(self.val["mySubFragments"]))

            case 'pattern_matcher::Fragment::Type::Alternative':
                return "(" + "|".join(ExtractSubFragments(self.val["mySubFragments"])) + ")"

            case 'pattern_matcher::Fragment::Type::Repeat':
                return NameOf(self.val["mySubFragments"]["_M_impl"]["_M_start"].dereference()) + " " + str(self.val["myCount"])[1:-1]

            case 'pattern_matcher::Fragment::Type::None':
                return "<Uninitialized>"

            case _:
                return 'Unknown Type: ' + fragType

class RepeatCountPrinter:
    "pattern_matcher::RepeatCount"

    def __init__(self, typename, val):
        self.val = val
        self.typename = typename
        self.enabled = True

    def to_string(self):
        # Make sure & works, too.
        type = self.val.type
        if type.code == gdb.TYPE_CODE_REF:
            type = type.target()

        lower = str(self.val['myMin'])
        upper = str(self.val['myMax'])

        if lower == upper:
            return "(" + lower + ")"

        if upper == '18446744073709551615':
            return "(" + lower + "+)"
        
        if lower == '0':
            return "(<=" + upper + ")"
        
        return "(" + lower + "-" + upper + ")"

def TemplateOf(name, numArguments):
    return "^" + name + "<" + ",".join([".*"] * numArguments) + ">$" # Todo: rework this to properly support templates (this gets tripped up by templates in templates as of now)

class Printer(gdb.printing.PrettyPrinter):
    def __init__(self, name):
        super(Printer, self).__init__(name)
        self.name = name
        self.subprinters = []
        self.lookup = []
        self.template_lookup = []
        self.enabled = True

    def add(self, name, function):
        print("[patternmatcher] Adding " + name)
        self.lookup.append((name, function))

    def add_template(self, name, function, numTemplateArguments):
        print("[patternmatcher] Adding template " + name)
        self.template_lookup.append((TemplateOf(name, numTemplateArguments), function)) 

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
                return printer(typename, val)

        
        for (regex, printer) in self.template_lookup:
            if re.search(regex, typename):
                return printer(typename, val)

        # Cannot find a pretty printer.  Return None.
        return None


MainPrinter = None

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

    MainPrinter.add_template('pattern_matcher::PatternMatcher', PatternMatcherPrinter, 1)
    MainPrinter.add('pattern_matcher::Fragment', FragmentPrinter)
    MainPrinter.add('pattern_matcher::RepeatCount', RepeatCountPrinter)

build_pattern_matcher_dictionary()
