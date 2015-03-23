
#AM_CXXFLAGS = -static
AM_CXXFLAGS = -Wall
#AM_CXXFLAGS += -pg
AM_CXXFLAGS += -O2 -funroll-loops -fexpensive-optimizations -DNDEBUG

# real program stuff
bin_PROGRAMS = hashbase

hashbase_SOURCES =
hashbase_SOURCES += hashfunc.hh hashfunc.cc
hashbase_SOURCES += label-map.hh label-map.cc
hashbase_SOURCES += hash.hh hash.cc

### Newick parser
hashbase_SOURCES += newick.h newick.c

# Mersenne Twister
hashbase_SOURCES += Random.cpp Random.hpp


hashbase_SOURCES += hashbase.cc

#eof
