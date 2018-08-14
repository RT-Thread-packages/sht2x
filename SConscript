
from building import *

cwd     = GetCurrentDir()
src     = Glob('*.c')
path    = [cwd]

group = DefineGroup('sht2x', src, depend = ['PKG_USING_SHT2X'], CPPPATH = path)

Return('group')
