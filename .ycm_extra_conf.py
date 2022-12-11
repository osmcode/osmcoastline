#-----------------------------------------------------------------------------
#
#  Configuration for YouCompleteMe Vim plugin
#
#  http://valloric.github.io/YouCompleteMe/
#
#-----------------------------------------------------------------------------

from os.path import realpath, dirname

basedir = dirname(realpath(__file__))

# some default flags
# for more information install clang-3.2-doc package and
# check UsersManual.html
flags = [
'-Werror',
'-Wall',
'-Wextra',
'-pedantic',
'-Wno-return-type',
'-Wno-unused-parameter',
'-Wno-unused-variable',

'-std=c++14',

# '-x' and 'c++' also required
# use 'c' for C projects
'-x',
'c++',

# workaround for https://github.com/Valloric/YouCompleteMe/issues/303
# also see https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=800618
'-isystem',
'/usr/lib/ycmd/clang_includes/',

'-Iinclude',
'-I%s/../libosmium/include' % basedir,
'-I/usr/include/gdal',

]

# youcompleteme is calling this function to get flags
# You can also set database for flags. Check: JSONCompilationDatabase.html in
# clang-3.2-doc package
def FlagsForFile( filename ):
  return {
    'flags': flags,
    'do_cache': True
  }
