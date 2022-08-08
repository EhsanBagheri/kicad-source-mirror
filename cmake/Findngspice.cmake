# CMake script for finding libngspice

# Copyright (C) 2016 CERN
# Copyright (C) 2020 Kicad Developers, see AUTHORS.txt for contributors.
#
# Author: Maciej Suminski <maciej.suminski@cern.ch>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# or you may search the http://www.gnu.org website for the version 2 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

# Usage:
#
# ngspice uses pkg-config to gather the configuration information so the pkg-config
# information is used to populate the CMake find_* commands.  When pkg-config is not
# available, the fallback is the default platform search paths set up by CMake.  The
# default search paths can be overridden by user.  The order of precedence is as
# follows:
#
# User defined root path NGSPICE_ROOT_DIR on CMake command line.
# User defined root path environment variable NGSPICE_ROOT_DIR.
# Paths generated by pkg-config.
# The default system paths configured by CMake
#
# For a specific ngspice include path not defined above set NGSPICE_INCLUDE_PATH.
#
# For a specific ngspice library path not defined above set NGSPICE_LIBRARY_PATH.

# Use pkg-config if it's available.
find_package(PkgConfig)

if( PKG_CONFIG_FOUND )
    pkg_check_modules( _NGSPICE ngspice )

    if( _NGSPICE_FOUND )
        set( NGSPICE_BUILD_VERSION "${_NGSPICE_VERSION}" CACHE STRING
             "ngspice version returned by pkg-config" )
    endif()
endif()

find_path( NGSPICE_INCLUDE_DIR ngspice/sharedspice.h
    PATHS
        ${NGSPICE_ROOT_DIR}
        $ENV{NGSPICE_ROOT_DIR}
        ${NGSPICE_INCLUDE_PATH}
        ${_NGSPICE_INCLUDE_DIRS}
    PATH_SUFFIXES
        include
        src/include
        share/ngspice/include
        share/ngspice/include/ngspice
)

if( UNIX )
    set( NGSPICE_LIB_NAME libngspice.so.0 CACHE STRING "Optionally versioned name of the shared library" )
else()
    set( NGSPICE_LIB_NAME ngspice CACHE STRING "Optionally versioned name of the shared library" )
endif()

find_library( NGSPICE_LIBRARY ${NGSPICE_LIB_NAME}
    PATHS
        ${NGSPICE_ROOT_DIR}
        $ENV{NGSPICE_ROOT_DIR}
        ${NGSPICE_LIBRARY_PATH}
        ${_NGSPICE_LIBRARY_DIRS}
    PATH_SUFFIXES
        src/.libs
        lib
)

# If the ngspice version is not set by pkg-config, see if ngspice/config.h is available.
if( NOT NGSPICE_BUILD_VERSION )
    find_file( NGSPICE_CONFIG_H ngspice/config.h
        PATHS
            ${NGSPICE_ROOT_DIR}
            $ENV{NGSPICE_ROOT_DIR}
            ${NGSPICE_INCLUDE_PATH}
            ${_NGSPICE_INCLUDE_DIRS}
        PATH_SUFFIXES
            include
            src/include
            share/ngspice/include
            share/ngspice/include/ngspice
        )

    if( NOT ${NGSPICE_CONFIG_H} STREQUAL "NGSPICE_CONFIG_H-NOTFOUND" )
        message( STATUS "ngspice configuration file ${NGSPICE_CONFIG_H} found." )
        set( NGSPICE_HAVE_CONFIG_H "1" CACHE STRING "ngspice/config.h header found." )
    endif()
endif()

if( WIN32 AND MSYS )
    # NGSPICE_LIBRARY points to libngspice.dll.a on Windows,
    # but the goal is to find out the DLL name.
    # Note: libngspice-0.dll or libngspice-1.dll must be in a executable path
    find_library( NGSPICE_DLL NAMES libngspice-0.dll libngspice-1.dll )

    # Some msys versions do not find xxx.dll lib files, only xxx.dll.a files
    # so try a find_file in exec paths
    if( NGSPICE_DLL STREQUAL "NGSPICE_DLL-NOTFOUND" )
        find_file( NGSPICE_DLL NAMES libngspice-0.dll libngspice-1.dll
            PATHS
                ${NGSPICE_ROOT_DIR}
            PATH_SUFFIXES
                bin
                lib
            )
    endif()

    if( NGSPICE_DLL STREQUAL "NGSPICE_DLL-NOTFOUND" )
        message( ERROR ":\n***** libngspice-x.dll not found in any executable path *****\n\n" )
    else()
        message( STATUS ": libngspice shared lib found: ${NGSPICE_DLL}\n" )
    endif()
elseif( WIN32 AND MSVC )
    find_file( NGSPICE_DLL NAMES ngspice.dll 
            PATHS
                ${NGSPICE_ROOT_DIR}
            PATH_SUFFIXES
                bin
                lib
            )
else()
    set( NGSPICE_DLL "${NGSPICE_LIBRARY}" )
endif()


include( FindPackageHandleStandardArgs )

if( ${NGSPICE_INCLUDE_DIR} STREQUAL "NGSPICE_INCLUDE_DIR-NOTFOUND" OR ${NGSPICE_LIBRARY} STREQUAL "NGSPICE_LIBRARY-NOTFOUND" )
    message( "" )
    message( "*** NGSPICE library missing ***" )
    message( "Most of ngspice packages do not provide the required libngspice library." )
    message( "You can either compile ngspice configured with --with-ngshared parameter" )
    message( "or run a script that does the job for you:" )
    message( "  cd ./tools/build" )
    message( "  chmod +x get_libngspice_so.sh" )
    message( "  ./get_libngspice_so.sh" )
    message( "  sudo ./get_libngspice_so.sh install" )
    message( "" )
endif()

find_package_handle_standard_args( ngspice
    REQUIRED_VARS NGSPICE_INCLUDE_DIR NGSPICE_LIBRARY NGSPICE_DLL )

mark_as_advanced(
    NGSPICE_INCLUDE_DIR
    NGSPICE_LIBRARY
    NGSPICE_DLL
    NGSPICE_BUILD_VERSION
    NGSPICE_HAVE_CONFIG_H
)
