#include "pywrap/binding/TypeObject.h"

#include "pywrap/binding/Tag.h"

namespace pywrap
{
namespace binding
{
TypeObject::TypeObject( const Tag& t ) : tag{ t }
{
	// Tag is not initialized yet
	// TypeObject should be initialized by its Tag
}

void TypeObject::gen_name()
{
	name << tag.get_py_name() << "_type_object";
}

void TypeObject::gen_py_name()
{
	py_name << get_name();
}

void TypeObject::gen_sign()
{
	sign << "PyTypeObject " << get_py_name();
}

void TypeObject::gen_decl()
{
	decl << "extern " << get_sign() << ";\n\n";
}

void TypeObject::gen_def()
{
	def << get_sign()
	    << " = {\n"
	       "\tPyVarObject_HEAD_INIT( NULL, 0 )\n\n"
	    << "\t\"" << tag.get_qualified_name() << "\", // name\n"
	    << "\tsizeof( _PyspotWrapper ), // basicsize\n"
	       "\t0, // itemsize\n\n"
	    << "\treinterpret_cast<destructor>( " << tag.get_destructor().get_name() << " ), // dealloc\n"
	    << "\t0, // print\n"
	       "\t0, // getattr\n"
	       "\t0, // setattr\n"
	       "\t0, // compare\n"
	       "\t0, // repr\n\n"
	       "\t0, // as_number\n"
	       "\t0, // as_sequence\n"
	       "\t0, // as_mapping\n\n"
	       "\t0, // hash\n"
	       "\t0, // call\n"
	       "\t0, // str\n"
	       "\t0, // getattro\n"
	       "\t0, // setattro\n\n"
	       "\t0, // as_buffer\n\n"
	       "\tPy_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // flags\n\n"
	    << "\t\"" << tag.get_qualified_name() << "\", // doc\n\n"
	    << "\t0, // traverse\n\n"
	       "\t0, // clear\n\n"
	    << "\treinterpret_cast<richcmpfunc>( " << tag.get_compare().get_name() << " ), // richcompare\n\n"
	    << "\t0, // weaklistoffset\n\n"
	       "\t0, // iter\n"
	       "\t0, // iternext\n\n"
	    << "\t" << tag.get_methods().get_py_name() << ", // methods\n"
	    << "\t" << tag.get_members().get_py_name() << ", // members\n"
	    << "\t" << tag.get_accessors().get_py_name() << ", // getset\n"
	    << "\t0, // base\n"
	       "\t0, // dict\n"
	       "\t0, // descr_get\n"
	       "\t0, // descr_set\n"
	       "\t0, // dictoffset\n"
	    << "\treinterpret_cast<initproc>( " << tag.get_init().get_name() << " ), // init\n"
	    << "\t0, // alloc\n"
	       "\tPyspotWrapper_New, // new\n};\n\n";
}
}  // namespace binding
}  // namespace pywrap