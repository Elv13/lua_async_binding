CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(tests)

FIND_PACKAGE(PkgConfig)

PKG_CHECK_MODULES(GLIB  REQUIRED glib-2.0)

INCLUDE_DIRECTORIES(${GLIB_INCLUDE_DIRS})

#
# The tests are individual binary because of their asynchronous aspect. A simple
# function with a return value cannot test the code. However, a full program
# return value do just fine as long as there is a timer preventing the mainloop
# from running forever
#

# Read file
ADD_EXECUTABLE( test_read1 test_read1.c )

TARGET_LINK_LIBRARIES( test_read1
   async
   ${GLIB_LIBRARIES}
)

# Scan/List directory
ADD_EXECUTABLE( test_scandir1 test_scandir1.c )

TARGET_LINK_LIBRARIES( test_scandir1
   async
   ${GLIB_LIBRARIES}
)

# Watch for file changes
ADD_EXECUTABLE( test_watch1 test_watch1.c )

TARGET_LINK_LIBRARIES( test_watch1
   async
   ${GLIB_LIBRARIES}
)

# Append text to a file
ADD_EXECUTABLE( test_append1 test_append1.c )

TARGET_LINK_LIBRARIES( test_append1
   async
   ${GLIB_LIBRARIES}
)

# Replace or create a text file
ADD_EXECUTABLE( test_write1 test_write1.c )

TARGET_LINK_LIBRARIES( test_write1
   async
   ${GLIB_LIBRARIES}
)