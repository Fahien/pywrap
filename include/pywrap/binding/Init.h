#ifndef PYWRAP_BINDING_INIT_H_
#define PYWRAP_BINDING_INIT_H_

#include <clang/AST/DeclCXX.h>

#include "pywrap/binding/Binding.h"

namespace pywrap
{
namespace binding
{
class Tag;

/// Represents bindings for an initializer of a Tag
class Init : public Binding
{
  public:
	/// Creates the bindings for an init
	/// @param[in] t Tag which this init belongs to
	Init( const Tag& );

	std::string get_def() const override;

  protected:
	void gen_name() override;
	void gen_sign() override;
	void gen_def() override;

  private:
	/// Adds a constructor to the definition of the initializer
	/// @param[in] constructor Constructor to support
	void add_def( const clang::CXXConstructorDecl& constructor );

	const Tag& tag;
};
}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDING_INIT_H_
