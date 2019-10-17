#include "pywrap/Util.h"
#include "pywrap/binding/CXXRecord.h"

namespace pywrap
{
void replace_all( std::string& str, const llvm::StringRef& from, const llvm::StringRef& to )
{
	size_t start_pos = 0;
	while ( ( start_pos = str.find( from, start_pos ) ) != std::string::npos )
	{
		str.replace( start_pos, from.size(), to );
		// Handles case where 'to' is a substring of 'from'
		start_pos += to.size();
	}
}


std::string replace_all( const std::string& str, const llvm::StringRef& from, const llvm::StringRef& to )
{
	std::string retstr{ str };
	replace_all( retstr, from, to );
	return retstr;
}


/// @param[in] name A c++ qualified name
/// @return A new string replacing every invalid character with a _ and putting a py_ at the beginning
std::string to_pyspot_name( std::string name )
{
	auto invalid = []( const char c ) { return c == ':' || c == ',' || c == '<' || c == '>'; };
	std::replace_if( std::begin( name ), std::end( name ), invalid, '_' );
	name.insert( 0, "py_" );
	return name;
}


std::string to_string( const clang::QualType& type, const TemplateMap& tMap, const clang::ASTContext& ctx )
{
	auto typ        = clang::TypeName::getFullyQualifiedType( type, ctx );
	auto typeString = typ.getAsString();

	std::string ret;
	std::string typeTail;
	if ( typ.isConstQualified() )
	{
		ret += "const ";
	}

	const clang::TemplateSpecializationType* pTemplType{ nullptr };

	if ( typ->isReferenceType() )
	{
		typ = typ.getNonReferenceType();
	}
	else if ( typ->isPointerType() )
	{
		// typeTail = "*";
		typ = typ->getPointeeType();
	}

	pTemplType = typ->getAs<clang::TemplateSpecializationType>();

	if ( pTemplType )
	{
		auto tail = typeString.substr( typeString.find_last_of( '>' ) + 1 );

		ret += typeString.substr( 0, typeString.find( '<' ) + 1 );
		for ( auto arg : pTemplType->template_arguments() )
		{
			auto argName = arg.getAsType().getUnqualifiedType().getAsString();  // T!

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


clang::QualType to_type( const clang::QualType& type, const TemplateMap& tMap )
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
		auto argName = tempType.getUnqualifiedType().getAsString();  // T!

		auto it = tMap.find( argName );
		assert( it != tMap.end() && "Template parameter not found" );

		tempType = it->second->getAsType();
	}

	return tempType;
}

std::string to_python( const clang::QualType& qual_type, std::string name )
{
	auto type = qual_type;

	// Pointer
	if ( type->isPointerType() )
	{
		type = type->getPointeeType();
		name = "( *" + name + " )";
	}

	// Check std classes
	std::string type_name = type.getUnqualifiedType().getAsString();
	if ( type_name.substr( 0, 5 ) == "class" )
	{
		type_name = type_name.substr( 6, type_name.size() - 6 );
	}

	// Bool
	if ( type->isBooleanType() )
	{
		return "PyBool_FromLong( static_cast<long>( " + name + ") )";
	}
	// Integer
	else if ( type->isIntegerType() )
	{
		return "PyLong_FromLong( static_cast<long>( " + name + " ) )";
	}
	// Float
	else if ( type->isFloatingType() )
	{
		return "PyFloat_FromDouble( static_cast<double>( " + name + " ) )";
	}
	// Char
	else if ( type->isCharType() )
	{
		return "PyUnicode_FromString( &" + name + " )";
	}
	// Array
	else if ( type->isArrayType() )
	{
		// Create python list
		std::string ret = "\tPyList_New( sizeof( " + name + " ) / sizeof( " + name + "[0] ) );\n";
		ret += "\tfor ( size_t i = 0; i < sizeof( " + name + " ) / sizeof( " + name + "[0] ); ++i )\n\t{\n\t\tauto& element = " + name + "[i];\n" +
		       "\t\tauto py_element = ";  // get param time

		auto array = type->getAsArrayTypeUnsafe();
		auto contained_type = array->getElementType();

		ret += to_python( contained_type , "element" );

		ret += ";\n\t\tPyList_SET_ITEM( ret, i, py_element );\n\t}";
		return ret;
	}
	// String
	else if ( type_name.find( "std::string" ) == 0 || type_name.find( "std::__1::basic_string<char>" ) == 0 )
	{
		return "PyUnicode_FromString( " + name + ".c_str() )";
	}
	// Vector
	else if ( type_name.find( "std::vector" ) == 0 )
	{
		// Create python list
		std::string ret = "PyList_New( " + name + ".size() );\n";
		ret += "\tfor ( size_t i = 0; i < " + name + ".size(); ++i )\n\t{\n" + "\t\tauto& element = " + name + "[i];\n" +
		       "\t\tauto py_element = ";  // get param time
		auto  vec_class      = qual_type->getAsCXXRecordDecl();
		auto  spec           = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>( vec_class );
		auto contained_type = spec->getTemplateArgs().get( 0 ).getAsType();

		ret += to_python( contained_type, "element" );

		ret += ";\n\t\tPyList_SET_ITEM( ret, i, py_element );\n\t}\n";
		return ret;
	}
	// Map
	else if ( type_name.find( "std::map" ) == 0 )
	{
		std::string ret = "PyDict_New();\n";
		ret += "\tfor ( auto& pair : " + name + " )\n\t{\n";

		auto  map_class      = qual_type->getAsCXXRecordDecl();
		auto  spec           = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>( map_class );
		auto key_type = spec->getTemplateArgs().get( 0 ).getAsType();
		auto val_type = spec->getTemplateArgs().get( 1 ).getAsType();

		ret += "\t\tauto py_key = " + to_python( key_type, "pair.first" ) + ";\n";
		ret += "\t\tauto py_val = " + to_python( val_type, "pair.second" ) + ";\n";
		ret += "\t\tPyDict_SetItem( ret, py_key, py_val );\n\t}\n";

		return ret;
	}
	// Object
	else
	{
		return "pyspot::Wrapper<" + type_name + ">{ const_cast<" + type_name + "*>( &" + name + " ) }.GetIncref()";
	}
}


std::string to_c( const clang::QualType& type, std::string name, std::string dest )
{
	auto actual_type = type;

	auto is_pointer = type->isPointerType();
	auto is_reference = type->isReferenceType();

	if ( is_pointer || is_reference )
	{
		actual_type = type->getPointeeType();
	}

	auto ret = dest + " = ";

	// Check std classes
	std::string type_name = actual_type.getUnqualifiedType().getAsString();
	if ( type_name.substr( 0, 5 ) == "class" )
	{
		type_name = type_name.substr( 6, type_name.size() - 6 );
	}

	// Bool
	if ( actual_type->isBooleanType() )
	{
		ret += "static_cast<bool>( PyLong_AsLong( " + name + " ) )";
	}
	// Integer
	else if ( actual_type->isIntegerType() )
	{
		if ( actual_type->isSpecificBuiltinType( clang::BuiltinType::Int ) )
		{
			ret += "static_cast<int>( PyLong_AsLong( " + name + " ) )";
		}
		else
		{
			ret += "PyLong_AsLong( " + name + " )";
		}
	}
	// Float
	else if ( actual_type->isSpecificBuiltinType( clang::BuiltinType::Float ) )
	{
		ret += "static_cast<float>( PyFloat_AsDouble( " + name + " ) )";
	}
	// Double
	else if ( actual_type->isSpecificBuiltinType( clang::BuiltinType::Double ) )
	{
		ret += "PyFloat_AsDouble( " + name + " )";
	}
	// Char
	else if ( actual_type->isCharType() )
	{
		ret += "pyspot::String{ " + name + " }.ToCString()";
	}
	// Array
	else if ( type->isArrayType() )
	{
		// Get elements from python list
		ret = "auto array_size = sizeof( " + dest + " ) / sizeof( " + dest + "[0] );\n";
		ret += "\tfor( size_t i = 0; i < array_size; ++i )\n\t{\n";
		ret += "\t\tauto element = PyList_GetItem( " + name + ", i );\n";
		auto array = type->getAsArrayTypeUnsafe();
		ret += "\t\t" + to_c( array->getElementType(), "element", dest + "[i]" ) + ";\n";
		ret += "\t}\n";
	}
	// String
	else if ( type_name.find( "std::string" ) == 0 || type_name.find( "std::__1::basic_string<char>" ) == 0 )
	{
		ret += "pyspot::String{ " + name + " }.ToCString()";
	}
	// Vector
	else if ( type_name.find( "std::vector" ) == 0 )
	{
		// Create python list
		auto  vec_class      = actual_type->getAsCXXRecordDecl();
		auto  spec           = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>( vec_class );
		auto& contained_type = spec->getTemplateArgs().get( 0 );
		// TODO fill vector
		ret += "std::vector<" + contained_type.getAsType().getAsString() + ">{}";
	}
	// Map
	else if ( type_name.find( "std::map" ) == 0 )
	{
		// Create python dictionary
		auto  map_class      = actual_type->getAsCXXRecordDecl();
		auto  spec           = clang::dyn_cast<clang::ClassTemplateSpecializationDecl>( map_class );
		auto key_type = spec->getTemplateArgs().get( 0 ).getAsType();
		auto val_type = spec->getTemplateArgs().get( 1 ).getAsType();

		// Get elements from python dict
		ret = dest + ".clear();\n\tPyObject* py_key;\n\tPyObject* py_val;\n\tPy_ssize_t pos = 0;\n";
		ret += "\twhile ( PyDict_Next( " + name + ", &pos, &py_key, &py_val ) )\n\t{\n";
		ret += "\t\tauto " + to_c( key_type, "py_key", "key" ) + ";\n";
		ret += "\t\tauto " + to_c( val_type, "py_val", "val" ) + ";\n";
		ret += "\t\t" + dest + "[key] = val;\n";
		ret += "\t}\n";
	}
	// Object
	else
	{
		auto assign = "reinterpret_cast<" + type_name + "*>( reinterpret_cast<_PyspotWrapper*>( " + name + " )->data )";

		// Get the pointer if that is expected
		if ( !is_pointer )
		{
			ret += "*";
		}

		ret += assign;
	}

	return ret;
}


std::string to_py_parser( const clang::QualType& type )
{
	if ( type->isIntegerType() )
	{
		return "i";
	}
	else if ( type->isFloatingType() )
	{
		return "f";
	}
	else if ( type->isUnsignedIntegerOrEnumerationType() )
	{
		return "I";
	}
	else if ( type->isRealFloatingType() )
	{
		return "d";
	}
	else
	{
		return "O";
	}
}


}  // namespace pywrap