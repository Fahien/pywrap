set( LLVM_LINK_COMPONENTS
	Support
)

set( PYWRAP pywrap )
add_clang_executable( ${PYWRAP}
	Pywrap.cpp
)

target_link_libraries( ${PYWRAP}
	PRIVATE
	clangAST
	clangASTMatchers
	clangBasic
	clangFrontend
	clangTooling
	clangToolingRefactor
)
