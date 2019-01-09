#ifndef PYSPOT_UTIL_H_
#define PYSPOT_UTIL_H_

#include <string>
#include <unordered_map>

#include <clang/AST/ASTContext.h>
#include "clang/AST/Type.h"
#include "clang/AST/Decl.h"
#include "clang/AST/QualTypeNames.h"


namespace pyspot
{

// TODO improve it using proper Types
using TemplateMap = std::unordered_map<std::string, clang::TemplateArgument*>;


template<typename T>
using Iterator = typename T::iterator;
template<typename T>
using ConstIterator = typename T::const_iterator;


template<typename Container, typename Type>
inline ConstIterator<Container> find( const Container& container, const Type& val )
{
	return std::find( std::begin( container ), std::end( container ), val );
}


inline void replace_all( std::string& str, const StringRef& from, const StringRef& to )
{
	size_t start_pos = 0;
	while ( ( start_pos = str.find( from, start_pos ) ) != std::string::npos )
	{
		str.replace( start_pos, from.size(), to );
		// Handles case where 'to' is a substring of 'from'
		start_pos += to.size();
	}
}


inline std::string replace_all( const std::string& str, const StringRef& from, const StringRef& to )
{
	std::string retstr { str };
	replace_all( retstr, from, to );
	return retstr;
}


inline std::string to_string( const clang::QualType& type, const TemplateMap& tMap, const clang::ASTContext& ctx )
{
	auto typ = clang::TypeName::getFullyQualifiedType( type, ctx );
	auto typeString = typ.getAsString();

	std::string ret;
	std::string typeTail;
	if ( typ.isConstQualified() )
	{
		ret += "const ";
	}

	const clang::TemplateSpecializationType* pTemplType { nullptr };

	if ( typ->isReferenceType() )
	{
		typ = typ.getNonReferenceType();
	}
	else if ( typ->isPointerType() )
	{
		//typeTail = "*";
		typ = typ->getPointeeType();
	}

	pTemplType = typ->getAs<clang::TemplateSpecializationType>();

	if ( pTemplType )
	{
		auto tail = typeString.substr( typeString.find_last_of( '>' ) + 1 );

		ret += typeString.substr( 0, typeString.find( '<' ) + 1 );
		for ( auto arg : pTemplType->template_arguments() )
		{
			auto argName = arg.getAsType().getUnqualifiedType().getAsString(); // T!

			auto it = tMap.find( argName );
			assert( it != tMap.end() && "Template parameter not found" );

			ret += it->second->getAsType().getAsString() + ",";
		}
		ret[ret.length() - 1] = '>';
		ret += typeTail + tail;
	}
	else if ( typ->isTemplateTypeParmType() )
	{
		auto it = tMap.find( typ.getUnqualifiedType().getAsString() );
		assert( it != tMap.end() && "Template parameter not found" );
		ret += it->second->getAsType().getAsString() + typeTail;
	}
	else
	{
		// Reset ret
		ret = typeString;
	}

	return ret;
}


inline clang::QualType to_type( const clang::QualType& type, const TemplateMap& tMap )
{
	auto tempType = type;

	if ( type->isReferenceType() )
	{
		tempType = type.getNonReferenceType();
	}
	else if ( type->isPointerType() )
	{
		tempType = type->getPointeeType();
	}

	if ( tempType->isTemplateTypeParmType() )
	{
		auto argName = tempType.getUnqualifiedType().getAsString(); // T!

		auto it = tMap.find( argName );
		assert( it != tMap.end() && "Template parameter not found" );

		tempType = it->second->getAsType();
	}

	return tempType;
}


inline std::string to_python( const clang::QualType& type, const std::string& name, const TemplateMap& tMap, const clang::ASTContext& ctx )
{
	auto realType = to_type( type, tMap );

	if ( realType->isBooleanType() )
	{
		return "PyBool_FromLong( static_cast<long>( " + name + ") )";
	}
	if ( realType->isIntegerType() )
	{
		return "PyLong_FromLong( static_cast<long>( " + name + " ) )";
	}
	if ( realType->isFloatingType() )
	{
		std::string ret = "PyFloat_FromDouble( static_cast<double>( ";
		if ( type->isPointerType() )
		{
			ret += "*";
		}
		ret += name + " ) )";
		return ret;
	}
	if ( realType->isPointerType() && realType->getPointeeType()->isCharType() )
	{
		return "PyUnicode_FromString( " + name + " )";
	}
	if ( realType.getAsString().find( "std::string" ) != std::string::npos )
	{
		return "PyUnicode_FromString( " + name + ".c_str() )";
	}
	else
	{
		auto ret = "pyspot::Wrapper<" + pyspot::to_string( type, tMap, ctx ) + ">{ ";
		if ( type->isReferenceType() )
		{
			ret += "&";
		}
		ret += name + " }.GetIncref()";
		return ret;
	}
}


inline std::string to_c( std::string type, const std::string& name )
{
	if ( type == "_Bool" )
	{
		return "static_cast<bool>( PyLong_AsLong( " + name + " ) )";
	}
	if ( type == "int" )
	{
		return "static_cast<int>( PyLong_AsLong( " + name + " ) )";
	}
	if ( type == "long" )
	{
		return "PyLong_AsLong( " + name + " )";
	}
	if ( type == "float" )
	{
		return "static_cast<float>( PyFloat_AsDouble( " + name + " ) )";
	}
	if ( type == "double" )
	{
		return "PyFloat_AsDouble( " + name + " )";
	}
	if ( type == "const char *" )
	{
		return "PyUnicode_AsUTF8( " + name + " )";
	}
	if ( type.find( "std::string" ) != std::string::npos )
	{
		return "PyUnicode_AsUTF8( " + name + " )";
	}
	else
	{
		if ( type.substr( 0, 5 ) == "class" )
		{
			type = type.substr( 6, type.size() - 6 );
		}
		return "*reinterpret_cast<" + type + "*>( reinterpret_cast<_PyspotWrapper*>( " + name + " )->pData )";
	}
}


inline std::string to_parser( const clang::QualType& type )
{
	if ( type->isIntegerType() )
	{
		return "i";
	}
	if ( type->isFloatingType() )
	{
		return "f";
	}
	if ( type->isUnsignedIntegerOrEnumerationType() )
	{
		return "I";
	}
	if ( type->isRealFloatingType() )
	{
		return "d";
	}

	return "O";
}



} // namespace pyspot

#endif // PYSPOT_UTIL_H_
