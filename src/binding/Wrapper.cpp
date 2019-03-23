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
	     << "pyspot::Wrapper<" << tag.get_qualified_name() << ">::Wrapper( " << tag.get_qualified_name() << "* v )";
}

void Wrapper::gen_def()
{
	auto type_object_name = tag.get_type_object().get_name();

	def << sign.str() << "\n"
	    << ":\tpyspot::Object {\n\t\t(\n"
	    << "\t\t\tPyType_Ready( &" << type_object_name << " ),\n"
	    << "\t\t\tPyspotWrapper_new( &" << type_object_name << ", nullptr, nullptr )\n\t\t)\n\t}\n"
	    << ",\tpayload { v }\n{\n"
	    << "\tauto wrapper = reinterpret_cast<_PyspotWrapper*>( object );\n"
	    << "\twrapper->data = payload;\n"
	    << "}\n\n";
}

}  // namespace binding
}  // namespace pywrap