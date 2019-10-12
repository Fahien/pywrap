set( LLVM_LINK_COMPONENTS Support )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )

add_clang_library( pywrap-lib
	${CMAKE_CURRENT_SOURCE_DIR}/src/Util.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/binding/Binding.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/binding/ClassGetitem.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/binding/Destructor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/binding/Init.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/binding/Compare.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/binding/Method.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/binding/TypeObject.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/binding/Field.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/binding/Tag.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/binding/Module.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/binding/Function.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/binding/Enum.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/binding/CXXRecord.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/binding/Wrapper.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Printer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/FrontendAction.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Consumer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/MatchHandler.cpp
)
add_clang_executable( pywrap
${CMAKE_CURRENT_SOURCE_DIR}/src/Pywrap.cpp
)


target_link_libraries( pywrap-lib
	PRIVATE
	clangAST
	clangASTMatchers
	clangBasic
	clangFrontend
	clangTooling
	clangToolingRefactoring
)

target_link_libraries( pywrap
	PRIVATE
	pywrap-lib
)
