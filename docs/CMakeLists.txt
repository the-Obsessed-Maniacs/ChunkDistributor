#[[
	docs folder of ChunkDistributor - a collection of MD files that should be contained in the
	deployed application.

	Stefan <St0fF> Kaps / the 0bsessed Maniacs / 2025
]]
cmake_minimum_required( VERSION 3.23 )

set( DOCS
	Algorithm.md
	UserInterface.md
	Collaborate.md
)
file( GLOB PIX RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "pix/*" )
target_sources( ${PROJECT_NAME}
	PUBLIC FILE_SET "documentation" TYPE "HEADERS" FILES ${DOCS} ${PIX}
)

##[[	custom target, shall produce a docs folder ]]
#add_custom_target( "${PROJECT_NAME}_docs" COMMENT "build docs folder" SOURCES ${DOCS} )
##[[ the actual command needs all files in certain path forms ]]
#list( APPEND DOC_SRC ${DOCS} )
#list( TRANSFORM DOC_SRC PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/" )
#list( APPEND DOC_TGT ${DOCS} )
#list( TRANSFORM DOC_TGT PREPEND "${CMAKE_CURRENT_BINARY_DIR}/" )
##[[ finally, add the command to the target ]]
#add_custom_command( COMMAND	${CMAKE_COMMAND} -E copy ${DOC_SRC} ${CMAKE_CURRENT_BINARY_DIR}
#	COMMAND_EXPAND_LISTS USES_TERMINAL VERBATIM
#	COMMENT "copy documents folder..."
#	DEPENDS ${DOCS}
#	TARGET ${PROJECT_NAME}_docs
#)
#add_dependencies( ${PROJECT_NAME} ${PROJECT_NAME}_docs )
#[[ I think I need to provide an install line for the docs, too ]]
