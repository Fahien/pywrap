#include "pywrap/binding/Tag.h"

#include "pywrap/Util.h"

namespace pywrap
{
namespace binding
{
Tag::Tag( const clang::TagDecl* t )
    : Binding{ t }, tag{ t }, qualified_name{ t->getQualifiedNameAsString() }, destructor{ *this }, type_object{ *this }
{
	// Leaves should init
}

void Tag::init()
{
	Binding::init();
	gen_reg();
	destructor.init();
	type_object.init();
}

std::string Tag::get_decl() const { return destructor.get_decl() + type_object.get_decl(); }

std::string Tag::get_def() const { return destructor.get_def() + type_object.get_def(); }

}  // namespace binding
}  // namespace pywrap
