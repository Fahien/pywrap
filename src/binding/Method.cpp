#include "pywrap/binding/Method.h"

#include "pywrap/binding/Function.h"
#include "pywrap/binding/Module.h"

namespace pywrap
{
namespace binding
{
Method::Method( const clang::CXXMethodDecl* m ) : Binding{ m }, method{ m }
{
	// Initialize when virtual methods are ready
	init();
}

void Method::gen_sign()
{
	// Return type is known
	sign << "PyObject* ";

	// Python name of the function with namespace
	sign << get_py_name();

	// Method parameters are always these
	sign << "( PyObject* self, PyObject* args, PyObject* kwds )";
}

void Method::gen_def()
{
	// TODO implementation
	def << sign.str() << "\n{}\n\n";
}

}  // namespace binding
}  // namespace pywrap