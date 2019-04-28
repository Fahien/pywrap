#include "pywrap/binding/Specialization.h"

namespace pywrap
{
namespace binding
{
Specialization::Specialization( const clang::ClassTemplateSpecializationDecl* s, const Binding& parent )
    : CXXRecord{ s, parent }, spec{ s }
{
}

void Specialization::gen_name()
{
	CXXRecord::gen_name();
	name << "<";
	auto args = spec->getTemplateInstantiationArgs().asArray();
	for ( size_t i = 0; i < args.size(); ++i )
	{
		auto& arg = args[i];
		name << arg.getAsType().getAsString();
		if ( i < args.size() - 1 )
		{
			name << ", ";
		}
	}
	name << ">";
}

}  // namespace binding
}  // namespace pywrap