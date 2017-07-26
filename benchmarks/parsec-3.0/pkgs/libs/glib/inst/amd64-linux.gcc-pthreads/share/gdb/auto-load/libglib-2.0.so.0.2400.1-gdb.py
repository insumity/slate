import sys
import gdb

# Update module path.
dir = '/localhome/kantonia/slate/benchmarks/parsec-3.0/pkgs/libs/glib/inst/amd64-linux.gcc-pthreads/share/glib-2.0/gdb'
if not dir in sys.path:
    sys.path.insert(0, dir)

from glib import register
register (gdb.current_objfile ())
