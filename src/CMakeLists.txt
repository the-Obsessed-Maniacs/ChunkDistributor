cmake_minimum_required( VERSION 3.16 )

if ( PROJECT_NAME STREQUAL "ChunkDistributor" )

set( PROJECT_SOURCES
    main.cpp
    PackerTester.ui
    PackerTester.h
    PackerTester.cpp
    PackerTester.rc
	Stage.h
	Stage.cpp
	AnimatePosition.h
	AnimatePosition.cpp
	Algo.h
	Algo.ui
	Algo.cpp
	AlgoData.h
	AlgoGfx.h
	AlgoGfx.cpp
	AlgoRunner0.h
	AlgoRunner0.cpp
)
set( PLUGIN_SOURCES
	AlgoData.h
	AlgoGfx.h AlgoGfx.cpp 
	Algo.ui Algo.h Algo.cpp
	DesignerPlugin.h DesignerPlugin.cpp 
	AlgoRunner0.h
	AlgoRunner0.cpp
)
qt_add_executable( FTest FTest.ui FTest.h FTest.cpp )
set_target_properties( FTest
    PROPERTIES
        WIN32_EXECUTABLE TRUE
)
target_link_libraries( FTest PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
	Qt::Charts
)
target_sources( DesignerPlugin PUBLIC ${PLUGIN_SOURCES} )
target_sources( ${PROJECT_NAME} PUBLIC ${PROJECT_SOURCES} )

endif()