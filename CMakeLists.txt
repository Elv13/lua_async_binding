CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

ADD_SUBDIRECTORY(lib)
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(src)

# Install in awesome XDG dir if the prefix is $HOME
IF (${CMAKE_INSTALL_PREFIX} STREQUAL $ENV{HOME} )
   SET(INSTALL_PATH $ENV{HOME}/.config/awesome/gears)
ELSE()
   SET(INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/share/awesome/lib/gears)
ENDIF()

MESSAGE(${CMAKE_CURRENT_BINARY_DIR})

# Create a working lua module called "async"
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/src/without_gtk/libluabridge.so  DESTINATION ${INSTALL_PATH}/async/)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/src/with_gtk/libluabridge_gtk.so DESTINATION ${INSTALL_PATH}/async/)
INSTALL(FILES src/init.lua DESTINATION ${INSTALL_PATH}/async/)