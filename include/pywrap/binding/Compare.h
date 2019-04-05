#ifndef PYWRAP_BINDING_COMPARE_H_
#define PYWRAP_BINDING_COMPARE_H_

#include "pywrap/binding/Binding.h"

namespace pywrap
{
namespace binding
{
class Tag;

/// Represents bindings for a compare function of a Tag
class Compare : public Binding
{
  public:
	/// Creates bindings for a compare function
	/// @param[in] t Tag which this compare belongs to
	Compare( const Tag& t );

  protected:
	void gen_name() override;
	void gen_sign() override;
	void gen_def() override;

  private:
	/// Generates operator equals
	void gen_eq();

	const Tag& tag;
};

}  // namespace binding
}  // namespace pywrap
#endif  // PYWRAP_BINDING_COMPARE_H_
