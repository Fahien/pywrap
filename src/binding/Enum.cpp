#include "pywrap/binding/Enum.h"

namespace pywrap
{
namespace binding
{
Enum::Enum( const clang::EnumDecl* e ) : Tag{ e }, enu{ e } { init(); }

void Enum::gen_reg()
{
	for ( auto value : enu->enumerators() )
	{
		auto name           = value->getNameAsString();
		auto qualified_name = get_qualified_name();

		reg << "\tPyDict_SetItemString( " << get_type_object().get_name() << ".tp_dict, \"" << name << "\", pyspot::Wrapper<"
		    << qualified_name << ">{ " << qualified_name << "::" << name << " }.GetIncref() );\n";
	}
}
}  // namespace binding
}  // namespace pywrap
