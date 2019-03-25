#include "pywrap/binding/Wrapper.h"

#include "pywrap/binding/Tag.h"


namespace pywrap
{
namespace binding
{
Wrapper::Wrapper( const Tag& t ) : tag{ t }
{
	// Initialized by the owning tag
}

void Wrapper::gen_sign()
{
	sign << "template<>\n"
	     << "pyspot::Wrapper<" << tag.get_qualified_name() << ">::Wrapper( ";
}

void Wrapper::gen_decl()
{
	// Pointer constructor
	decl << sign.str() << tag.get_qualified_name() << "* v );\n\n";
	// Copy constructor
	decl << sign.str() << "const " << tag.get_qualified_name() << "& v );\n\n";
	// Move constructor
	decl << sign.str() << tag.get_qualified_name() << "&& v );\n\n";
}

void Wrapper::gen_pointer_constructor_def()
{
	auto type_object_name = tag.get_type_object().get_name();

	// Pointer constructor
	def << sign.str() << tag.get_qualified_name() << "* v )\n"
	    << ":\tpyspot::Object {\n\t\t(\n"
	    << "\t\t\tPyType_Ready( &" << type_object_name << " ),\n"
	    << "\t\t\tPyspotWrapper_new( &" << type_object_name << ", nullptr, nullptr )\n\t\t)\n\t}\n"
	    << ",\tpayload { v }\n{\n"
	    << "\tauto wrapper = reinterpret_cast<_PyspotWrapper*>( object );\n"
	    << "\twrapper->data = payload;\n"
	    << "}\n\n";
}

void Wrapper::gen_copy_constructor_def()
{
	auto type_object_name = tag.get_type_object().get_name();

	// Pointer constructor
	def << sign.str() << "const" << tag.get_qualified_name() << "& v )\n"
	    << ":\tpyspot::Object {\n\t\t(\n"
	    << "\t\t\tPyType_Ready( &" << type_object_name << " ),\n"
	    << "\t\t\tPyspotWrapper_new( &" << type_object_name << ", nullptr, nullptr )\n\t\t)\n\t}\n"
	    << ",\tpayload { new " << tag.get_qualified_name() << " { v } }\n{\n"
	    << "\tauto wrapper = reinterpret_cast<_PyspotWrapper*>( object );\n"
	    << "\twrapper->data = payload;\n"
		<< "\twrapper->own_data = true;\n"
	    << "}\n\n";
}

void Wrapper::gen_move_constructor_def()
{
	auto type_object_name = tag.get_type_object().get_name();

	def << sign.str() << tag.get_qualified_name() << "&& v )\n"
	    << ":\tpyspot::Object {\n\t\t(\n"
	    << "\t\t\tPyType_Ready( &" << type_object_name << " ),\n"
	    << "\t\t\tPyspotWrapper_new( &" << type_object_name << ", nullptr, nullptr )\n\t\t)\n\t}\n"
	    << ",\tpayload { new " << tag.get_qualified_name() << " { std::move( v ) } }\n{\n"
	    << "\tauto wrapper = reinterpret_cast<_PyspotWrapper*>( object );\n"
	    << "\twrapper->data = payload;\n"
		<< "\twrapper->own_data = true;\n"
	    << "}\n\n";
}

void Wrapper::gen_def()
{
	gen_pointer_constructor_def();
	gen_copy_constructor_def();
	gen_move_constructor_def();
}

}  // namespace binding
}  // namespace pywrap