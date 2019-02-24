set( LLVM_LINK_COMPONENTS Support )

add_clang_executable( pywrap
	src/Printer.cpp
	src/FrontendAction.cpp
	src/Consumer.cpp
	src/MatchHandler.cpp
	src/Pywrap.cpp
)

target_include_directories( pywrap PUBLIC include )

target_link_libraries( pywrap PRIVATE
	clangAST
	clangASTMatchers
	clangBasic
	clangFrontend
	clangTooling
	clangToolingRefactor
)
