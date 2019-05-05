#include "pywrap/binding/CXXRecord.h"

namespace pywrap
{
namespace binding
{
CXXRecord::CXXRecord( const clang::CXXRecordDecl& rec, const Binding& parent ) : Tag{ rec, parent }, record{ rec }
{
}


Specialization::Specialization( const clang::ClassTemplateSpecializationDecl& s, const Binding& parent )
    : CXXRecord{ s, parent }, spec{ s }
{
	gen_template_args();
}

void Specialization::gen_template_args()
{
	// Add template arguments
	template_args << "<";
	auto args = spec.getTemplateInstantiationArgs().asArray();
	for ( size_t i = 0; i < args.size(); ++i )
	{
		auto& arg = args[i];
		template_args << arg.getAsType().getAsString();
		if ( i < args.size() - 1 )
		{
			template_args << ",";
		}
	}
	template_args << ">";
}

void Specialization::gen_name()
{
	CXXRecord::gen_name();
	name << template_args.str();
}

void Specialization::gen_qualified_name()
{
	get_mut_qualified_name() += template_args.str();
}


Template::Template( const clang::ClassTemplateDecl& t, const Binding& parent ) : Tag{ t, parent }, templ{ t }
{
}


void Template::add( const Specialization& spec )
{
	specializations.push_back( &spec );
}


}  // namespace binding
}  // namespace pywrap
