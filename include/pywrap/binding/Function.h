#ifndef PYWRAP_BINDING_FUNCTION_H_
#define PYWRAP_BINDING_FUNCTION_H_

#include <sstream>

#include <clang/AST/Decl.h>

#include "pywrap/binding/Binding.h"

namespace pywrap
{
namespace binding
{
class Function : public Binding
{
  public:
	/// Generated definition and declaration of Python bindings for a function
	/// @param[in] func Function to wrap
	Function( const clang::FunctionDecl* func );

	Function( Function&& ) = default;

	/// @return The clang FunctionDecl
	const clang::FunctionDecl* get_func() const { return func; }

  protected:
	/// @return A signature of the binding
	void gen_sign() override;

	/// @return A definition of the bindings
	void gen_def() override;

  private:
	/// Function decl
	const clang::FunctionDecl* func;
};

}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDING_FUNCTION_H_
