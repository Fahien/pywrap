#include "pywrap/binding/Tag.h"

#include "pywrap/Util.h"
#include "pywrap/binding/CXXRecord.h"

namespace pywrap
{
namespace binding
{
Tag::Methods::Methods( const Tag* t ) : tag{ t }
{
	// Will be initialized by the tag
}


void Tag::Methods::gen_py_name()
{
	if ( !tag )
	{
		py_name << "0";
		return;
	}

	py_name << tag->get_py_name() << "_methods";
}


void Tag::Methods::gen_sign()
{
	if ( tag )
	{
		sign << "PyMethodDef " << py_name.str();
	}
}


void Tag::Methods::gen_def()
{
	if ( tag )
	{
		def << sign.str() << "[] = {\n";
		if ( auto templ = dynamic_cast<const Template*>( tag ) )
		{
			size++;
			def << "\t{ \"__class_getitem__\", " << templ->get_class_getitem().get_py_name()
			    << ", METH_O|METH_CLASS, NULL },\n";
		}
	}
}


std::string Tag::Methods::get_decl() const
{
	if ( !tag )
	{
		return "";
	}

	return "extern " + sign.str() + "[" + std::to_string( size ) + "];\n";
}


std::string Tag::Methods::get_def() const
{
	if ( !tag )
	{
		return "";
	}

	return def.str() + "\t{ NULL, NULL, 0, NULL } // sentinel\n};\n\n";
}


const char* gen_meth( const Method& m )
{
	if ( m->param_size() == 0 )
	{
		return "METH_NOARGS";
	}
	else
	{
		return "METH_VARARGS | METH_KEYWORDS";
	}
}


void Tag::Methods::add( const Method& method )
{
	if ( tag )
	{
		def << "\t{ \"" << method.get_name() << "\", " << method.get_py_name() << ", " << gen_meth( method ) << ", \""
		    << method.get_name() << "\" },\n";
	}
}


Tag::Members::Members( const Tag* t ) : tag{ t }
{
	// Will be initialized by the tag
}


void Tag::Members::gen_py_name()
{
	if ( !tag )
	{
		py_name << "0";
		return;
	}

	py_name << tag->get_py_name() << "_members";
}


void Tag::Members::gen_sign()
{
	if ( tag )
	{
		sign << "PyMemberDef " << py_name.str();
	}
}


void Tag::Members::gen_def()
{
	if ( tag )
	{
		// Just definition
		def << sign.str() << "[] = {\n"
		    << "\t{ NULL } // sentinel\n};\n\n";
	}
}


std::string Tag::Members::get_decl() const
{
	if ( !tag )
	{
		return "";
	}

	return "extern " + sign.str() + "[" + std::to_string( size ) + "];\n\n";
}


Tag::Accessors::Accessors( const Tag* t ) : tag{ t }
{
	// Will be initialized by the tag
}


void Tag::Accessors::gen_py_name()
{
	if ( !tag )
	{
		py_name << "0";
		return;
	}

	py_name << tag->get_py_name() << "_accessors";
}


void Tag::Accessors::gen_sign()
{
	if ( tag )
	{
		sign << "PyGetSetDef " << py_name.str();
	}
}


void Tag::Accessors::gen_def()
{
	if ( tag )
	{
		// Just definition
		def << sign.str() << "[] = {\n";

		if ( auto record = dynamic_cast<const CXXRecord*>( tag ) )
		{
			size += record->get_fields().size();

			for ( auto& field : record->get_fields() )
			{
				def << "\t{ \"" << field.get_name() << "\", reinterpret_cast<getter>( " << field.get_getter().get_name()
				    << " ), reinterpret_cast<setter>( " << field.get_setter().get_name() << " ), \"" << field.get_name()
				    << "\", nullptr },\n";
			}
		}

		def << "\t{ NULL } // sentinel\n};\n\n";
	}
}


std::string Tag::Accessors::get_decl() const
{
	if ( tag )
	{
		return "extern " + sign.str() + "[" + std::to_string( size ) + "];\n\n";
	}
	return "";
}


Tag::Tag( Tag&& o )
    : Binding{ std::move( o ) }
    , reg{ std::move( o.reg ) }
    , tag{ o.tag }
    , templ{ o.templ }
    , destructor{ std::move( o.destructor ) }
    , initializer{ std::move( o.initializer ) }
    , compare{ std::move( o.compare ) }
    , class_getitem{ std::move( o.class_getitem ) }
    , methods{ std::move( o.methods ) }
    , members{ std::move( o.members ) }
    , accessors{ std::move( o.accessors ) }
    , type_object{ std::move( o.type_object ) }
    , wrapper{ std::move( o.wrapper ) }
{
	initializer.tag = this;
}


Tag::Tag( const clang::TagDecl& t, const Binding& p )
    : Binding{ &t, &p }
    , tag{ &t }
    , destructor{ *this }
    , initializer{ *this }
    , compare{ this }
    , methods{ this }
    , members{ this }
    , accessors{ this }
    , type_object{ *this }
    , wrapper{ this }
{
}


Tag::Tag( const clang::ClassTemplateDecl& t, const Binding& p )
    : Binding{ &t, &p }
    , templ{ &t }
    , destructor{ *this }
    , initializer{ *this }
    , class_getitem{ this }
    , methods{ this }
    , type_object{ *this }
{
}

void Tag::init()
{
	// Should be initialized after construction
	Binding::init();
	destructor.init();
	initializer.init();
	compare.init();
	class_getitem.init();
	methods.init();
	members.init();
	gen_fields();
	accessors.init();
	type_object.init();
	wrapper.init();
	gen_reg();
}


void Tag::gen_reg()
{
	auto type_object_name = type_object.get_name();

	reg << "\tif ( PyType_Ready( &" << type_object_name << " ) < 0 )\n"
	    << "\t{\n\t\treturn nullptr;\n\t}\n"
	    << "\tPy_INCREF( &" << type_object_name << " );\n"
	    << "\tPyModule_AddObject( " << parent->get_py_name() << ", \"" << get_name() << "\", "
	    << "reinterpret_cast<PyObject*>( &" << type_object_name << " ) );\n\n";
}


std::string Tag::get_decl() const
{
	// These declarations will go within extern "C"
	// Wrapper decl should not go there
	return destructor.get_decl() + initializer.get_decl() + compare.get_decl() + class_getitem.get_decl() +
	       methods.get_decl() + members.get_decl() + accessors.get_decl() + type_object.get_decl();
}


std::string Tag::get_def() const
{
	return destructor.get_def() + initializer.get_def() + compare.get_def() + class_getitem.get_def() + methods.get_def() +
	       members.get_def() + accessors.get_def() + type_object.get_def() + wrapper.get_def();
}


}  // namespace binding
}  // namespace pywrap
