CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

ADD_DEFINITIONS("-std=c++11")

PROJECT(libasync_gtk)

ADD_DEFINITIONS("-D ENABLE_GTK=true")

FIND_PACKAGE(PkgConfig)

PKG_CHECK_MODULES(GTK3  REQUIRED gtk+-3.0)
PKG_CHECK_MODULES(GLIB  REQUIRED glib-2.0)
PKG_CHECK_MODULES(GIO   REQUIRED gio-2.0 )
PKG_CHECK_MODULES(CAIRO REQUIRED cairo   )
PKG_CHECK_MODULES(PANGO REQUIRED pango   )
PKG_CHECK_MODULES(GDK   REQUIRED gdk-3.0 )

INCLUDE_DIRECTORIES(${GTK3_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES(${GLIB_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES(${GIO_INCLUDE_DIRS}  )
INCLUDE_DIRECTORIES(${CAIRO_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${PANGO_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${GDK_INCLUDE_DIRS}  )

ADD_DEFINITIONS(${GTK3_CFLAGS} )
ADD_DEFINITIONS(${GLIB_CFLAGS} )
ADD_DEFINITIONS(${GIO_CFLAGS}  )
ADD_DEFINITIONS(${CAIRO_CFLAGS})
ADD_DEFINITIONS(${PANGO_CFLAGS})
ADD_DEFINITIONS(${GDK_CFLAGS}  )

INCLUDE_DIRECTORIES(..)

SET(libasync_SRCS
   ../async.C
)

ADD_LIBRARY( async_gtk SHARED ${libasync_SRCS} )

TARGET_LINK_LIBRARIES( async_gtk
   ${GLIB_LIBRARIES}
   ${GIO_LIBRARIES}
   ${CAIRO_LIBRARIES}
   ${PANGO_LIBRARIES}
   ${GDK_LIBRARIES}
   ${GTK3_LIBRARIES}
)
