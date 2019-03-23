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
	/// Generates the signature of the binding
	virtual void gen_sign() override;

	/// Generates the definition of the bindings
	virtual void gen_def() override;

  private:
	/// Function decl
	const clang::FunctionDecl* func;
};

}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDING_FUNCTION_H_
