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
	Wrapper( const Tag& t );

  protected:
	/// Generates the signature
	virtual void gen_sign() override;

	/// Generates Wrapper definitions
	virtual void gen_def() override;

  private:
	/// Wrapped Tag
	const Tag& tag;
};

}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDING_WRAPPER_H_
