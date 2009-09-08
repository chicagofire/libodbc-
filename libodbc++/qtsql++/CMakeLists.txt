project(qtsqlxx)
find_package(Qt4 4.4.3 COMPONENTS QtCore QtGui REQUIRED )
set(QT_USE_QTMAIN true)
include(${QT_USE_FILE})

set (MOC_HEADERS
    connectwindow.h
    mainwindow.h
    resultwindow.h
    )
    
set (SOURCES 
    connectwindow.cpp
    main.cpp
    mainwindow.cpp
    resultwindow.cpp
)

QT4_WRAP_CPP( MOC_SOURCES ${MOC_HEADERS} )

link_directories(odbcxx_BINARY_DIR)
include_directories(${QT_INCLUDES})

add_executable( ${PROJECT_NAME} WIN32 ${MOC_HEADERS} ${SOURCES} ${MOC_SOURCES})
target_link_libraries( ${PROJECT_NAME} ${QT_LIBRARIES} odbcxx)

