#include "pywrap/binding/Field.h"

#include "pywrap/Util.h"
#include "pywrap/binding/CXXRecord.h"

namespace pywrap
{
namespace binding
{
void Getter::gen_name()
{
	name << field->get_tag().get_py_name() << "_get_" << field->get_name();
}

void Getter::gen_sign()
{
	sign << "PyObject* " << name.str() << "( _PyspotWrapper* self, void* /*closure*/ )";
}

void Getter::gen_def()
{
	def << sign.str() << "\n{\n"
	    << "\tauto data = reinterpret_cast<" << field->get_tag().get_qualified_name() << "*>( self->data );\n"
	    << "\tauto ret = " << to_python( field->get_type(), "data->" + field->get_name() ) << ";\n"
	    << "\treturn ret;\n"
	    << "}\n\n";
}

void Setter::gen_name()
{
	name << field->get_tag().get_py_name() << "_set_" << field->get_name();
}

void Setter::gen_sign()
{
	sign << "int " << name.str() << "( _PyspotWrapper* self, PyObject* value, void* /*closure*/ )";
}

void Setter::gen_def()
{
	def << sign.str() << "\n{\n"
	    << "\tif ( !value )\n\t{\n"
	    << "\t\tPyErr_SetString( PyExc_TypeError, \"Cannot delete " << name.str() << "\" );\n"
	    << "\t\treturn -1;\n\t}\n\n"
	    << "\tauto data = reinterpret_cast<" << field->get_tag().get_qualified_name() << "*>( self->data );\n"
	    << "\tdata->" << field->get_name() << " = " << to_c( field->get_type(), "value" ) << ";\n"
	    << "\treturn 0;\n}\n\n";
}

Field::Field( const clang::FieldDecl& f, const Tag& t )
    : field{ f }, tag{ t }, name{ field.getNameAsString() }, getter{ *this }, setter{ *this }
{
}

Field::Field( Field&& other )
    : field{ other.field }
    , tag{ other.tag }
    , name{ std::move( other.name ) }
    , getter{ std::move( other.getter ) }
    , setter{ std::move( other.setter ) }
{
	getter.field = this;
	setter.field = this;
}

const clang::QualType Field::get_type() const
{
	if ( field.isTemplated() )
	{
		if ( auto spec = dynamic_cast<const Specialization*>( &tag ) )
		{
			if ( auto arg = spec->get_arg( field ) )
			{
				return arg->getAsType();
			}
		}
	}

	return field.getType();
}

}  // namespace binding
}  // namespace pywrap
