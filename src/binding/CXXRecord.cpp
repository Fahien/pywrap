#include "pywrap/binding/CXXRecord.h"

namespace pywrap
{
namespace binding
{
CXXRecord::CXXRecord( const clang::CXXRecordDecl& rec, const Binding& parent ) : Tag{ rec, parent }, record{ rec }
{
}


void CXXRecord::gen_fields()
{
	for ( auto field : record.fields() )
	{
		auto is_public = ( field->getAccess() == clang::AS_public );
		if ( !is_public )
		{
			continue;
		}

		add_field( Field{ *field, *this } );
	}
}


void CXXRecord::add_field( Field&& f )
{
	f.init();
	fields.emplace_back( std::move( f ) );
}


void CXXRecord::init()
{
	// Should be initialized after construction
	Tag::init();
}


std::string CXXRecord::get_decl() const
{
	std::string ret;

	for ( auto& field : fields )
	{
		ret += field.get_getter().get_decl() + field.get_setter().get_decl();
	}

	return ret + Tag::get_decl();
}


std::string CXXRecord::get_def() const
{
	std::string ret;

	for ( auto& field : fields )
	{
		ret += field.get_getter().get_def() + field.get_setter().get_def();
	}

	return ret + Tag::get_def();
}


Specialization::Specialization( const clang::ClassTemplateSpecializationDecl& s, const Binding& parent )
    : CXXRecord{ s, parent }, spec{ s }, args{ spec.getTemplateInstantiationArgs().asArray() }
{
	gen_template_args();
}

void Specialization::gen_template_args()
{
	// Add template arguments
	template_args << "<";
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
	CXXRecord::gen_qualified_name();
	get_mut_qualified_name() << template_args.str();
}


void Specialization::gen_fields()
{
	auto record = spec.getSpecializedTemplate()->getTemplatedDecl();
	for ( auto field : record->fields() )
	{
		auto is_public = ( field->getAccess() == clang::AS_public );
		if ( !is_public )
		{
			continue;
		}

		add_field( Field{ *field, *this } );
	}
}

const clang::TemplateArgument* Specialization::get_arg( const clang::FieldDecl& f ) const
{
	auto templ_params    = spec.getSpecializedTemplate()->getTemplateParameters();
	auto field_type_name = f.getType().getAsString();

	for ( size_t i = 0; i < args.size(); ++i )
	{
		auto param = templ_params->getParam( i );
		if ( param->getName() == field_type_name )
		{
			return &args[i];
		}
	}

	return nullptr;
}


Template::Template( const clang::ClassTemplateDecl& t, const Binding& parent ) : Tag{ t, parent }, templ{ t }
{
}


void Template::add( const Specialization& spec )
{
	get_mut_class_getitem().add( spec );
	specializations.push_back( &spec );
}


}  // namespace binding
}  // namespace pywrap
