
from building import *

cwd     = GetCurrentDir()
src     = Glob('*.c') + Glob('*.cpp')
CPPPATH = [cwd]

group = DefineGroup('sht2x', src, depend = [''], CPPPATH = CPPPATH)

Return('group')
