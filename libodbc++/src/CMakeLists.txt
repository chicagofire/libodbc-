project(odbcxx)

set ( HEADER 
    datahandler.h
    driverinfo.h
    dtconv.h
    ../include/odbc++/callablestatement.h
    ../include/odbc++/config-win32.h
    ../include/odbc++/config.h.in
    ../include/odbc++/connection.h
    ../include/odbc++/databasemetadata.h
    ../include/odbc++/errorhandler.h
    ../include/odbc++/preparedstatement.h
    ../include/odbc++/resultset.h
    ../include/odbc++/resultsetmetadata.h
    ../include/odbc++/setup.h
    ../include/odbc++/statement.h
    ../include/odbc++/threads.h
    ../include/odbc++/types.h
)

set ( SOURCES 
    callablestatement.cpp
    connection.cpp
    databasemetadata.cpp
    datahandler.cpp
    datastream.cpp
    datetime.cpp
    driverinfo.cpp
    drivermanager.cpp
    errorhandler.cpp
    preparedstatement.cpp
    resultset.cpp
    resultsetmetadata.cpp
    statement.cpp
    threads.cpp
)

add_definitions(  -DWIN32 -DODBCXX_DLL -DIN_ODBCXX)
add_library( ${PROJECT_NAME} SHARED ${HEADER} ${SOURCES} )

target_link_libraries( ${PROJECT_NAME} "odbc32.lib")
