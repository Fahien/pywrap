#ifndef PYWRAP_BINDINGS_MODULE_H_
#define PYWRAP_BINDINGS_MODULE_H_

#include <sstream>

#include <clang/AST/Decl.h>

#include "pywrap/binding/Binding.h"
#include "pywrap/binding/Function.h"

namespace pywrap
{
namespace binding
{
class Module : public Binding
{
  public:
	class Methods : public Binding
	{
	  public:
		Methods( const Module& m );

		void add( const Function& function );

		void close();

	  protected:
		/// @return The python name of the binding
		void gen_py_name() override;
	
		/// @return A definition of the bindings
		void gen_def() override;

	  private:
		const Module& module;

		bool closed;
	};

	/// A module binding consist of an init function declaration
	Module( const clang::NamespaceDecl* n );

	void add( Function&& function );

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
	/// Namespace decl
	const clang::NamespaceDecl* ns;

	/// Python MethodDef
	Methods methods;
};

}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDINGS_MODULE_H_
