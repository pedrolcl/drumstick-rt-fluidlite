# Drumstick RT (realtime MIDI In/Out) FluidLite Backend
# Copyright (C) 2009-2022 Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.14)
project( drumstick-rt-fluidlite
    VERSION 2.7
    LANGUAGES CXX
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

option(STATIC_DRUMSTICK "Build a static plugin instead of a share one" OFF)

find_package(QT NAMES Qt5 Qt6 REQUIRED)
if ((CMAKE_SYSTEM_NAME MATCHES "Linux") AND (QT_VERSION_MAJOR EQUAL 6) AND (QT_VERSION VERSION_LESS 6.4))
    message(WARNING "Unsupported Qt version ${QT_VERSION} for system ${CMAKE_SYSTEM_NAME}")
endif()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Multimedia)
find_package(Drumstick 2.7 REQUIRED COMPONENTS RT)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/FluidLite)
include(${CMAKE_CURRENT_SOURCE_DIR}/FluidLite/aliases.cmake)

set(DRUMSTICK_PLUGINS_DIR "drumstick2")
#message(STATUS "DRUMSTICK PLUGINS destination directory: ${CMAKE_INSTALL_FULL_LIBDIR}/${DRUMSTICK_PLUGINS_DIR}")
#get_target_property(DRUMSTICK_INTERFACES Drumstick::RT INTERFACE_INCLUDE_DIRECTORIES)
#message(STATUS "DRUMSTICK INCLUDE INTERFACES: ${DRUMSTICK_INTERFACES}")
#get_target_property(FLUIDLITE_INTERFACES fluidlite::fluidlite INTERFACE_INCLUDE_DIRECTORIES)
#message(STATUS "FLUIDLITE INCLUDE INTERFACES: ${FLUIDLITE_INTERFACES}")

set(DUMMY_OUT_SOURCES 
    fluidcontroller.cpp
    fluidcontroller.h
    fluidliteoutput.cpp
    fluidliteoutput.h
    fluidrenderer.cpp
    fluidrenderer.h
    fluidsettingsdialog.cpp
    fluidsettingsdialog.h
    fluidsettingsdialog.ui
)

if(STATIC_DRUMSTICK)
    add_library(drumstick-rt-fluidlite STATIC ${DUMMY_OUT_SOURCES})
    target_compile_definitions(drumstick-rt-fluidlite PRIVATE QT_STATICPLUGIN VERSION=${PROJECT_VERSION})
else()
    add_library(drumstick-rt-fluidlite MODULE ${DUMMY_OUT_SOURCES})
    target_compile_definitions(drumstick-rt-fluidlite PRIVATE QT_PLUGIN VERSION=${PROJECT_VERSION})
endif()

target_link_libraries(drumstick-rt-fluidlite PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
    Drumstick::RT
    fluidlite::fluidlite
)

install(TARGETS drumstick-rt-fluidlite
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${DRUMSTICK_PLUGINS_DIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${DRUMSTICK_PLUGINS_DIR}
)
