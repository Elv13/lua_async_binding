CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(tests)

FIND_PACKAGE(PkgConfig)

ADD_DEFINITIONS("-D ENABLE_GTK=true")

PKG_CHECK_MODULES(GLIB REQUIRED glib-2.0)

INCLUDE(FindLua)
INCLUDE(FindPkgConfig)

INCLUDE_DIRECTORIES(
   ${GLIB_INCLUDE_DIRS}
   ${LUA_INCLUDE_DIR}
)

ADD_DEFINITIONS(-fPIC)

INCLUDE_DIRECTORIES(..)

# Read file
ADD_LIBRARY( luabridge_gtk SHARED ../luabridge.c )

TARGET_LINK_LIBRARIES( luabridge_gtk
   async_gtk
   ${LUA_LIBRARIES}
   ${GLIB_LIBRARIES}
)
