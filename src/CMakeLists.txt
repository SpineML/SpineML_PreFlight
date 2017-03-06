add_library(spinemlpreflight STATIC
component.cpp connection_list.cpp experiment.cpp fixedvalue.cpp
modelpreflight.cpp normaldistribution.cpp propertycontent.cpp rng.cpp
timepointvalue.cpp uniformdistribution.cpp util.cpp valuelist.cpp
)

add_executable(spineml_preflight spineml_preflight.cpp)
target_link_libraries(spineml_preflight spinemlpreflight ${POPT_LIBRARY})

add_executable(ebd_float2double ebd_float2double.cpp)
target_link_libraries(ebd_float2double spinemlpreflight ${POPT_LIBRARY})

add_executable(testutil testutil.cpp)
target_link_libraries(testutil spinemlpreflight ${POPT_LIBRARY})

add_executable(testuniformdistribution testuniformdistribution.cpp)
target_link_libraries(testuniformdistribution spinemlpreflight ${POPT_LIBRARY})

install(
  PROGRAMS
  ${CMAKE_CURRENT_BINARY_DIR}/spineml_preflight
  ${CMAKE_CURRENT_BINARY_DIR}/ebd_float2double
  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(FILES spineml_preflight.1 ebd_float2double.1
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
