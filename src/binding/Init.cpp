#include "pywrap/binding/Init.h"

#include "pywrap/Util.h"
#include "pywrap/binding/Tag.h"

namespace pywrap
{
namespace binding
{
Init::Init( const Tag& t ) : tag{ t }
{
	// Initialized by the tag
}

void Init::gen_name()
{
	name << tag.get_py_name() << "_init";
}

void Init::gen_sign()
{
	sign << "int " << name.str() << "( _PyspotWrapper* self, PyObject* args, PyObject* kwds )";
}

void Init::gen_def()
{
	// Template is trivial
	if ( tag.get_templ() )
	{
		def << sign.str() << "\n{\n\treturn 0;\n};\n\n";
		return;
	}

	def << sign.str() << "\n{\n"
	    << "\t" << tag.get_qualified_name() << "* data = nullptr;\n"
	    << "\tif ( self->data )\n\t{\n"
	    << "\t\tdata = reinterpret_cast<" << tag.get_qualified_name() << "*>( self->data );\n"
	    << "\t\treturn 0;\n\t}\n\n";

	// Get args and kwds size
	def << "\tauto args_size = args ? PyTuple_Size( args ) : 0;\n"
	       "\tauto kwds_size = kwds ? PyDict_Size( kwds ) : 0;\n\n";

	auto record = clang::dyn_cast<clang::CXXRecordDecl>( tag.get_handle() );

	// Default constructor
	if ( !record || record->hasDefaultConstructor() )
	{
		def << "\tif ( args_size == 0 && kwds_size == 0 )\n"
		    << "\t{\n\t\tdata = new " << tag.get_qualified_name() << "{};\n"
		    << "\t\tself->data = data;\n"
		       "\t\tself->own_data = true;\n"
		       "\t\treturn 0;\n\t}\n\n";
	}

	// Add constructors
	if ( record )
	{
		for ( const auto& method : record->methods() )
		{
			if ( auto constructor = clang::dyn_cast<clang::CXXConstructorDecl>( method ) )
			{
				if ( !constructor->isCopyOrMoveConstructor() )
				{
					add_def( *constructor );
				}
			}
		}
	}
}

void Init::add_def( const clang::CXXConstructorDecl& constructor )
{
	// Calculates args and kwds size
	int args_count = 0;  // signed for count will become negative
	int kwds_count = 0;

	for ( auto param : constructor.parameters() )
	{
		param->hasDefaultArg() ? ++kwds_count : ++args_count;
	}

	// Open if
	def << "\tif ( ( args_size + kwds_size ) == " << constructor.parameters().size() << " )\n\t{\n";

	// Create kwlist and fmt
	auto kwlist_name = std::string{ "kvlist" };
	auto fmt_name    = std::string{ "fmt" };

	std::stringstream kwlist_def;
	kwlist_def << "char* " << kwlist_name << "[] { ";

	std::stringstream fmt_def;
	fmt_def << "const char* " << fmt_name << " { \"";

	std::stringstream args_pointers;
	std::stringstream call_args;

	for ( auto param : constructor.parameters() )
	{
		if ( args_count == 0 )
		{
			// Separator
			fmt_def << "|";
		}
		--args_count;

		// Parser for fmt
		auto qual_type = param->getType();
		qual_type.removeLocalConst();
		fmt_def << pywrap::to_py_parser( qual_type );

		auto param_name = param->getNameAsString();
		args_pointers << ", &" << param_name;

		// Param decl
		def << "\t\t";
		if ( qual_type->isBuiltinType() )
		{
			def << qual_type.getAsString() << " ";
			call_args << param_name << ", ";
		}
		else
		{
			def << "PyObject* ";
			call_args << pywrap::to_c( param->getType(), param_name ) << ", ";
		}
		def << param_name << " {};\n";

		// Update kwlist
		auto param_name_name = param_name + "_name";
		def << "\t\tchar " << param_name_name << "[] { \"" << param_name << "\" };\n\n";
		kwlist_def << param_name_name << ", ";
	}

	// Put separator on empty kwds
	if ( args_count == 0 )
	{
		fmt_def << "|";
	}

	// Remove last , from callargs
	auto call_args_str = call_args.str();
	if ( call_args_str.size() > 2 )
	{
		call_args_str = call_args_str.substr( 0, call_args_str.size() - 2 );
	}

	def << "\t\t" << kwlist_def.rdbuf() << "nullptr };\n";
	def << "\t\t" << fmt_def.rdbuf() << "\" };\n\n";

	// Parse tuple and keywords
	def << "\t\tif ( PyArg_ParseTupleAndKeywords( args, kwds, " << fmt_name << ", " << kwlist_name << args_pointers.str()
	    << " ) )\n\t\t{\n";

	// Call constructor
	def << "\t\t\tdata = new " << tag.get_qualified_name() << "{ " << call_args_str << " };\n"
	    << "\t\t\tself->data = data;\n\t\t\tself->own_data = true;\n";

	def << "\t\t\treturn 0;\n\t\t}\n";

	// Close if
	def << "\t}\n";
}

std::string Init::get_def() const
{
	auto ret = def.str();
	if ( tag.get_templ() )
	{
		return ret;
	}
	return ret + "\treturn -1;\n}\n\n";
}
}  // namespace binding
}  // namespace pywrap
