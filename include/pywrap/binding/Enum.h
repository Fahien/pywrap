#ifndef PYWRAP_BINDING_ENUM_H_
#define PYWRAP_BINDING_ENUM_H_

#include "pywrap/binding/Tag.h"

namespace pywrap
{
namespace binding
{
/// Represents the Python bindings of an Enum
class Enum : public Tag
{
  public:
	/// Generates definition and declaration of Python bindings for an enum
	/// @param[in] enu Enum to wrap
	Enum( const clang::EnumDecl& enu, const Binding& parent );

  protected:
	/// Generates the registration of the bindings
	void gen_reg() override;

  private:
	/// Enum decl
	const clang::EnumDecl& enu;
};
}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDING_ENUM_H_
