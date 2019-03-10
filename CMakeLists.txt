set( LLVM_LINK_COMPONENTS Support )

add_clang_library( pywrap-lib
	src/binding/Binding.cpp
	src/binding/Destructor.cpp
	src/binding/Compare.cpp
	src/binding/TypeObject.cpp
	src/binding/Tag.cpp
	src/binding/Module.cpp
	src/binding/Function.cpp
	src/binding/Enum.cpp
	src/Printer.cpp
	src/FrontendAction.cpp
	src/Consumer.cpp
	src/MatchHandler.cpp
)
add_clang_executable( pywrap
	src/Pywrap.cpp
)

target_include_directories( pywrap-lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include )
target_include_directories( pywrap PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include )

target_link_libraries( pywrap-lib
	PRIVATE
	clangAST
	clangASTMatchers
	clangBasic
	clangFrontend
	clangTooling
	clangToolingRefactor
)

target_link_libraries( pywrap
	PRIVATE
	pywrap-lib
)