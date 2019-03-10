#include "pywrap/binding/Tag.h"

#include "pywrap/Util.h"

namespace pywrap
{
namespace binding
{
Tag::Tag( const clang::TagDecl* t )
    : Binding{ t },
      tag{ t },
      qualified_name{ t->getQualifiedNameAsString() },
      destructor{ *this },
      compare{ *this },
      type_object{ *this }
{
	// Leaves should init
}

void Tag::init()
{
	Binding::init();
	destructor.init();
	compare.init();
	type_object.init();
	gen_reg();
}

std::string Tag::get_decl() const { return destructor.get_decl() + compare.get_decl() + type_object.get_decl(); }

std::string Tag::get_def() const { return destructor.get_def() + compare.get_def() + type_object.get_def(); }

}  // namespace binding
}  // namespace pywrap
