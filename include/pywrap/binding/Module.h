#ifndef PYWRAP_BINDINGS_MODULE_H_
#define PYWRAP_BINDINGS_MODULE_H_

#include <sstream>

#include <clang/AST/Decl.h>

#include "pywrap/binding/Enum.h"
#include "pywrap/binding/Function.h"

namespace pywrap
{
namespace binding
{
/// This represents the bindings for a Python module
class Module : public Binding
{
  public:
	/// This represents a PyMethodDef structure
	/// responsible to hold the methods associated to a module
	class Methods : public Binding
	{
	  public:
		Methods( const Module& m );

		Methods( Methods&& ) = default;

		/// Adds a function to the methods map
		/// @param f The function to add
		void add( const Function& f );

		/// @return The definition of the methods map
		std::string get_def() const override;

	  protected:
		/// @return The python name of the methods map
		void gen_py_name() override;

		/// @return Definition of the methods map
		void gen_def() override;

	  private:
		const Module& module;
	};

	/// A module binding consist of an init function declaration
	Module( const clang::NamespaceDecl* n );

	Module( Module&& ) = default;

	std::string get_def() const override;

	/// Adds a function to this module
	/// @param[in] f The function to add to this module
	void add( Function&& f );

	/// Adds an enum to this module
	/// @param[in] e The enum to add to this module
	void add( Enum&& e );

	/// @return The methods map for the module init function
	const Methods& get_methods() const { return methods; }

	/// @return Functions associated with the module
	const std::vector<Function>& get_functions() const { return functions; }

	/// @return Enums associated with the module
	const std::vector<Enum>& get_enums() const { return enums; }

  protected:
	/// @return The Python name of the binding
	virtual void gen_py_name() override;

	/// @return A signature of the binding
	virtual void gen_sign() override;

	/// @return A definition of the bindings
	virtual void gen_def() override;

  private:
	/// Namespace decl
	const clang::NamespaceDecl* ns;

	/// Python MethodDef
	Methods methods;

	/// Module functions
	std::vector<Function> functions;

	/// Module enums
	std::vector<Enum> enums;
};

}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDINGS_MODULE_H_
