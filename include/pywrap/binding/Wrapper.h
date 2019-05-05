#ifndef PYWRAP_BINDING_WRAPPER_H_
#define PYWRAP_BINDING_WRAPPER_H_

#include "pywrap/binding/Binding.h"


namespace pywrap
{
namespace binding
{
class Tag;

class Wrapper : public Binding
{
  public:
	/// Creates Wrapper specializations for a Tag
	/// @param[in] t Tag to wrap
	Wrapper( const Tag* t = nullptr );

  protected:
	/// Generates the signature
	void gen_sign() override;

	/// Generates the declarations
	void gen_decl() override;

	/// Generates Wrapper definitions
	void gen_def() override;

  private:
	/// Generates pointer constructor def
	void gen_pointer_constructor_def();

	/// Generates copy constructor def
	void gen_copy_constructor_def();

	/// Generates move constructor def
	void gen_move_constructor_def();

	/// Wrapped Tag
	const Tag* tag;
};

}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDING_WRAPPER_H_
