INCLUDES=-I$(top_srcdir)/include/rapidxml-1.13

bin_PROGRAMS = spineml_preflight

spineml_preflight_SOURCES = spineml_preflight.cpp connection_list.cpp modelpreflight.cpp \
                            propertycontent.cpp fixedvalue.cpp uniformdistribution.cpp \
                            normaldistribution.cpp valuelist.cpp component.cpp rng.cpp \
                            experiment.cpp util.cpp
spineml_preflight_LDADD = -lpopt
