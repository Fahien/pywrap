#ifndef PYWRAP_BINDING_DESTRUCTOR_H_
#define PYWRAP_BINDING_DESTRUCTOR_H_

#include "pywrap/binding/Binding.h"

namespace pywrap
{
namespace binding
{
class Tag;

/// Represents bindings for a destructor of a Tag
class Destructor : public Binding
{
  public:
	/// Creates the bindings for a destructor
	/// @param[in] t Tag which this destructor belongs to
	Destructor( const Tag& t );

  protected:
	void gen_name() override;
	void gen_sign() override;
	void gen_def() override;

  private:
	const Tag& tag;
};
}  // namespace binding
}  // namespace pywrap
#endif  // PYWRAP_BINDING_DESTRUCTOR_H_
