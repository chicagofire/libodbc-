
if(WIN32)
if(MSVC)
set(READLINE_FOLDER "C:/Program Files/GnuWin32")
set(READLINE_LIB "${READLINE_FOLDER}/lib/readline.lib" "${READLINE_FOLDER}/lib/history.lib")
set(READLINE_INCLUDE "${READLINE_FOLDER}/include")
endif(MSVC)
endif(WIN32)


set (SOURCES isql++.cpp)
set (HEADERS isql++.h)

include_directories("../src" ${READLINE_INCLUDE})
add_definitions( )
add_executable( isql++ ${HEADERS} ${SOURCES})
target_link_libraries( isql++ libodbc++  ${READLINE_LIB})

