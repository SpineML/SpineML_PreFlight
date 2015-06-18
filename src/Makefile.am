AM_CPPFLAGS=-I$(top_srcdir)/include/rapidxml-1.13

bin_PROGRAMS = spineml_preflight ebd_float2double

spineml_preflight_SOURCES = spineml_preflight.cpp connection_list.cpp modelpreflight.cpp \
                            propertycontent.cpp fixedvalue.cpp uniformdistribution.cpp \
                            normaldistribution.cpp valuelist.cpp component.cpp rng.cpp \
                            experiment.cpp util.cpp timepointvalue.cpp
spineml_preflight_LDADD = -lpopt

ebd_float2double_SOURCES = ebd_float2double.cpp connection_list.cpp modelpreflight.cpp \
                            propertycontent.cpp fixedvalue.cpp uniformdistribution.cpp \
                            normaldistribution.cpp valuelist.cpp component.cpp rng.cpp \
                            experiment.cpp util.cpp timepointvalue.cpp
ebd_float2double_LDADD = -lpopt
