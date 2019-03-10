#ifndef PYWRAP_BINDING_TYPEOBJECT_H_
#define PYWRAP_BINDING_TYPEOBJECT_H_

#include "pywrap/binding/Binding.h"

namespace pywrap
{
namespace binding
{

class Tag;

class TypeObject : public Binding
{
  public:
	TypeObject( const Tag& t );

  protected:
	/// @return The name of the binding
	void gen_name() override;

	/// @return The Python name of the binding
	void gen_py_name() override;

	/// @return A signature of the binding
	void gen_sign() override;

	/// @return A declaration of the bindings
	void gen_decl() override;

	/// @return A definition of the bindings
	void gen_def() override;

  private:
	const Tag& tag;
};
}
}

#endif // PYWRAP_BINDING_TYPEOBJECT_H_
