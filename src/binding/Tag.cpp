#include "pywrap/binding/Tag.h"

#include "pywrap/Util.h"

namespace pywrap
{
namespace binding
{
Tag::Methods::Methods( const Tag& t ) : tag{ t }
{
	// Will be initialized by the tag
}

void Tag::Methods::gen_py_name()
{
	py_name << tag.get_py_name() << "_methods";
}

void Tag::Methods::gen_sign()
{
	sign << "PyMethodDef " << py_name.str();
}

void Tag::Methods::gen_def()
{
	def << sign.str() << "[] = {\n";
}

std::string Tag::Methods::get_decl() const
{
	return "extern " + sign.str() + "[" + std::to_string( size ) + "];\n";
}

std::string Tag::Methods::get_def() const
{
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
	def << "\t{ \"" << method.get_name() << "\", " << method.get_py_name() << ", " << gen_meth( method ) << ", \""
	    << method.get_name() << "\" },\n";
}

Tag::Members::Members( const Tag& t ) : tag{ t }
{
	// Will be initialized by the tag
}

void Tag::Members::gen_py_name()
{
	py_name << tag.get_py_name() << "_members";
}

void Tag::Members::gen_sign()
{
	sign << "PyMemberDef " << py_name.str();
}

void Tag::Members::gen_def()
{
	// Just definition
	def << sign.str() << "[] = {\n"
	    << "\t{ NULL } // sentinel\n};\n\n";
}

std::string Tag::Members::get_decl() const
{
	return "extern " + sign.str() + "[" + std::to_string( size ) + "];\n\n";
}

Tag::Accessors::Accessors( const Tag& t ) : tag{ t }
{
	// Will be initialized by the tag
}

void Tag::Accessors::gen_py_name()
{
	py_name << tag.get_py_name() << "_accessors";
}

void Tag::Accessors::gen_sign()
{
	sign << "PyGetSetDef " << py_name.str();
}

void Tag::Accessors::gen_def()
{
	// Just definition
	def << sign.str() << "[] = {\n"
	    << "\t{ NULL } // sentinel\n};\n\n";
}

std::string Tag::Accessors::get_decl() const
{
	return "extern " + sign.str() + "[" + std::to_string( size ) + "];\n\n";
}


Tag::Tag( const clang::TagDecl* t )
    : Binding{ t }
    , tag{ t }
    , qualified_name{ t->getQualifiedNameAsString() }
    , destructor{ *this }
    , initializer{ *this }
    , compare{ *this }
    , methods{ *this }
    , members{ *this }
    , accessors{ *this }
    , type_object{ *this }
    , wrapper{ *this }
{
	// Leaves should init
}

void Tag::init()
{
	Binding::init();
	destructor.init();
	initializer.init();
	compare.init();
	methods.init();
	members.init();
	accessors.init();
	type_object.init();
	wrapper.init();
	gen_reg();
}

std::string Tag::get_decl() const
{
	return destructor.get_decl() + initializer.get_decl() + compare.get_decl() + methods.get_decl() + members.get_decl() +
	       accessors.get_decl() + type_object.get_decl() + wrapper.get_decl();
}

std::string Tag::get_def() const
{
	return destructor.get_def() + initializer.get_def() + compare.get_def() + methods.get_def() + members.get_def() +
	       accessors.get_def() + type_object.get_def() + wrapper.get_def();
}

}  // namespace binding
}  // namespace pywrap
