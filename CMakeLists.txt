set( LLVM_LINK_COMPONENTS
	Support
)

set( PYWRAP pywrap )
add_clang_executable( ${PYWRAP}
	src/Printer.cpp
	src/FrontendAction.cpp
	src/Consumer.cpp
	src/MatchHandler.cpp
	src/Pywrap.cpp
)

target_include_directories( ${PYWRAP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include )

target_link_libraries( ${PYWRAP}
	PRIVATE
	clangAST
	clangASTMatchers
	clangBasic
	clangFrontend
	clangTooling
	clangToolingRefactor
)
