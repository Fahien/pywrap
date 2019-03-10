#ifndef PYWRAP_BINDING_METHOD_H_
#define PYWRAP_BINDING_METHOD_H_

#include <clang/AST/DeclCXX.h>

#include "pywrap/binding/Binding.h"

namespace pywrap
{
namespace binding
{
/// This represents a method structure associated to a Tag
class Method : public Binding
{
  public:
	Method( const clang::CXXMethodDecl* method );

	Method( Method&& ) = default;

	/// @return The clang method decl
	const clang::CXXMethodDecl* operator->() const { return method; }

  protected:
	/// Generates the signature of the method
	void gen_sign() override;

	/// Generates the definition of the method bindings
	void gen_def() override;

  private:
	/// CXX method decl
	const clang::CXXMethodDecl* method;
};

}  // namespace binding
}  // namespace pywrap
#endif  // PYWRAP_BINDING_METHOD_H_
