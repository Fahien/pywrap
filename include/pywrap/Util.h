#ifndef PYSPOT_UTIL_H_
#define PYSPOT_UTIL_H_

#include <string>
#include <unordered_map>

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/QualTypeNames.h>
#include <clang/AST/Type.h>


namespace pywrap
{
// TODO improve it using proper Types
using TemplateMap = std::unordered_map<std::string, clang::TemplateArgument*>;


template <typename T>
using Iterator = typename T::iterator;
template <typename T>
using ConstIterator = typename T::const_iterator;


template <typename Container, typename Type>
static ConstIterator<Container> find( const Container& container, const Type& val )
{
	return std::find( std::begin( container ), std::end( container ), val );
}

void replace_all( std::string& str, const llvm::StringRef& from, const llvm::StringRef& to );


std::string replace_all( const std::string& str, const llvm::StringRef& from, const llvm::StringRef& to );


/// @param[in] name A c++ qualified name
/// @return A new string replacing every invalid character with a _ and putting a py_ at the beginning
std::string to_pyspot_name( std::string name );


std::string to_string( const clang::QualType& type, const TemplateMap& tMap, const clang::ASTContext& ctx );


clang::QualType to_type( const clang::QualType& type, const TemplateMap& tMap );


std::string to_python( const clang::QualType& type, std::string name );


std::string to_python( const clang::QualType& type, const std::string& name, const TemplateMap& tMap,
                       const clang::ASTContext& ctx );

std::string to_c( const clang::QualType& type, std::string name );


std::string to_c( std::string type, std::string name );


std::string to_py_parser( const clang::QualType& type );


}  // namespace pywrap

#endif  // PYSPOT_UTIL_H_
