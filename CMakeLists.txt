cmake_minimum_required( VERSION 3.23 )
project( ChunkDistributor LANGUAGES CXX )

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

find_package( QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core )
find_package( Qt${QT_VERSION_MAJOR}
    COMPONENTS
        Core
        Gui
        Widgets
		UiPlugin
)
qt_standard_project_setup()
qt_add_plugin( DesignerPlugin )
qt_add_executable( ${PROJECT_NAME} )
qt_add_resources( ${PROJECT_NAME} PREFIX "/" BASE ${CMAKE_CURRENT_SOURCE_DIR}
	FILES ReadMe.md
)
add_subdirectory( "src" )
add_subdirectory( "docs" )

source_group( "00_SRCS" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/src/.*[.]c(pp)?" )
source_group( "01_HDRS" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/src/.*[.]h" )
source_group( "02_RSCS" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/src/.*[.]((ui)|(rc)|(qrc))" )
source_group( "03_DOCS" REGULAR_EXPRESSION ".*[.]((md)|(png))" )

target_link_libraries( DesignerPlugin PUBLIC
    Qt::Core
    Qt::Gui
    Qt::UiPlugin
    Qt::Widgets
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        WIN32_EXECUTABLE TRUE
)
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

cmake_path( SET INSTALL_PLUGDIR NORMALIZE "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS}/designer")
message( NOTICE "QT6 designer install dir: ${INSTALL_PLUGDIR}")

install( TARGETS DesignerPlugin
    RUNTIME DESTINATION "${INSTALL_PLUGDIR}"
    BUNDLE DESTINATION "${INSTALL_PLUGDIR}"
    LIBRARY DESTINATION "${INSTALL_PLUGDIR}"
)
if ( WIN32 )
	install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION .
		FILE_SET "documentation" DESTINATION "doc"
	)
else()
	install(TARGETS ${PROJECT_NAME}
		BUNDLE  DESTINATION .
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		FILE_SET "documentation" DESTINATION "doc"
	)
endif()
install( FILES ReadMe.md DESTINATION . )

qt_generate_deploy_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR NO_TRANSLATIONS
)
install(SCRIPT ${deploy_script})
