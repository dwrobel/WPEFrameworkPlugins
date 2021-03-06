set(PLUGIN_NAME OCDM)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(ocdm REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)

# TODO: set PLUGIN_OPENCDMI_PLAYREADY_READ_DIR default value to empty when flag will be provided from the BR
set(PLUGIN_OPENCDMI_PLAYREADY_READ_DIR "/root/Netflix/dpi/playready" CACHE STRING "Playready read-dir")
set(PLUGIN_OPENCDMI_PLAYREADY_STORE_LOCATION "/root/Netflix/dpi/playready/storage/drmstore" CACHE STRING "Playready store-location")

add_library(${MODULE_NAME} SHARED 
        OCDM.cpp
        OCDMJsonRpc.cpp
        CENCParser.cpp
        FrameworkRPC.cpp
        Module.cpp)

# avoid -as-needed flag being set, this will break linking to libocdm.so
string(REPLACE "-Wl,--as-needed" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-as-needed")

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME} 
        PRIVATE
                ${NAMESPACE}Plugins::${NAMESPACE}Plugins 
                ocdm::ocdm)
# Library definition section
install(TARGETS ${MODULE_NAME} 
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

if("${CMAKE_FIND_ROOT_PATH}" STREQUAL "")
   # Desktop case: not cross compiling
   target_link_libraries(${MODULE_NAME} PRIVATE "${CMAKE_INSTALL_PREFIX}/lib/libocdm.so")
else()
   # Cross compiling: buildroot
   target_link_libraries(${MODULE_NAME} PRIVATE ${PLUGINS_LIBRARIES} ${OCDM_LIBRARIES})
endif()

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config(${PLUGIN_NAME})
