#ifndef PYWRAP_BINDING_H_
#define PYWRAP_BINDING_H_

#include <sstream>

#include <clang/AST/Decl.h>

namespace pywrap {
namespace binding {

class Binding
{
  public:
	std::string get_name() const { return name.str(); }

	std::string get_py_name() const { return py_name.str(); }

	std::string get_decl() const { return decl.str(); }

	std::string get_def() const { return def.str(); }

  protected:
	/// Generates the name of the binding
	virtual void gen_name() {};

	/// Generates the Python name of the binding
	virtual void gen_py_name() {};

	/// Generates the signature of the binding
	virtual void gen_sign() {};

	/// Generates the declaration of the bindings
	virtual void gen_decl() {};

	/// Generates the definition of the bindings
	virtual void gen_def() {};

	/// Name
	std::stringstream name;

	/// Python name
	std::stringstream py_name;

	/// Signature
	std::stringstream sign;

	/// Declaration
	std::stringstream decl;

	/// Definition
	std::stringstream def;
};

} // namespace binding
} // namespace pywrap

#endif // PYWRAP_BINDING_H_