add_library(spinemlpreflight STATIC
component.cpp connection_list.cpp experiment.cpp fixedvalue.cpp
modelpreflight.cpp normaldistribution.cpp propertycontent.cpp rng.cpp 
timepointvalue.cpp uniformdistribution.cpp util.cpp valuelist.cpp
)

add_executable(spineml_preflight spineml_preflight.cpp)
target_link_libraries(spineml_preflight spinemlpreflight ${POPT_LIBRARY})

add_executable(ebd_float2double ebd_float2double.cpp)
target_link_libraries(ebd_float2double spinemlpreflight ${POPT_LIBRARY})

install(PROGRAMS spineml_preflight ebd_float2double DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

