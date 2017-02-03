cmake_minimum_required(VERSION 2.6)

project(DREAMlib)

if (WIN32)
ADD_DEFINITIONS(-DWIN32 -D_WIN32 -D_WINDOWS)
ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
endif (WIN32)

cmake_policy(SET CMP0042 NEW)  # CMake 3.0 ``MACOSX_RPATH`` is enabled by default.

### Require C++11 ###
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

###############################################################
#Bootstrap search for libraries 
# (We need to find cmake modules in Gadgetron)
###############################################################
find_path(GADGETRON_CMAKE_MODULES FindGadgetron.cmake HINTS
$ENV{GADGETRON_HOME}/share/gadgetron/cmake
/usr/local/gadgetron)

if (NOT GADGETRON_CMAKE_MODULES)
  MESSAGE(FATAL_ERROR "GADGETRON_CMAKE_MODULES cannot be found. 
   Try to set GADGETRON_HOME environment variable.")
endif(NOT GADGETRON_CMAKE_MODULES)

set(CMAKE_MODULE_PATH ${GADGETRON_CMAKE_MODULES})
###############################################################

find_package(ISMRMRD REQUIRED)
find_package(Gadgetron REQUIRED)
find_package(Boost REQUIRED)
find_package(ACE REQUIRED)

set(CMAKE_INSTALL_PREFIX ${GADGETRON_HOME})

INCLUDE_DIRECTORIES(${ACE_INCLUDE_DIR} 
     ${Boost_INCLUDE_DIR}
     ${GADGETRON_INCLUDE_DIR}
     ${ISMRMRD_INCLUDE_DIR}
     ${ISMRMRD_SCHEMA_DIR}
     ${ISMRMRD_XSD_INCLUDE_DIR}
     )

LINK_DIRECTORIES(${GADGETRON_LIB_DIR})

ADD_LIBRARY(gadgetronDREAMlib SHARED
            phasePreserveRxGadget.cpp
            wtcFFTGadget.cpp
            wtcSplitGadget.cpp
            wtcDREAMGadget.cpp
            wtcPTxDREAMGadget.cpp
            wtcExtractGadget.cpp
            faToB1Gadget.cpp)

TARGET_LINK_LIBRARIES(gadgetronDREAMlib 
                      gadgetron_gadgetbase
                      gadgetron_toolbox_log
                      optimized ${ACE_LIBRARIES} 
                      debug ${ACE_DEBUG_LIBRARY})

INSTALL (FILES phasePreserveRxGadget.h
         wtcFFTGadget.h
         wtcSplitGadget.h
         wtcDREAMGadget.h
         wtcPTxDREAMGadget.h
         wtcExtractGadget.h
         faToB1Gadget.h
         DREAMlib_export.h
         DESTINATION include)

INSTALL(TARGETS gadgetronDREAMlib DESTINATION lib)

if (NOT GADGETRON_INSTALL_CONFIG_PATH)
set(GADGETRON_INSTALL_CONFIG_PATH  ${GADGETRON_HOME}/share/gadgetron/config)
endif ()
INSTALL(FILES ptxDREAM.xml stxDREAM.xml DESTINATION ${GADGETRON_INSTALL_CONFIG_PATH})
