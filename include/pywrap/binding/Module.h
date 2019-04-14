#ifndef PYWRAP_BINDINGS_MODULE_H_
#define PYWRAP_BINDINGS_MODULE_H_

#include <sstream>

#include <clang/AST/Decl.h>

#include "pywrap/binding/CXXRecord.h"
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
	Module( const clang::NamespaceDecl* n, const Binding* parent = nullptr );

	Module( Module&& ) = default;

	const clang::NamespaceDecl* get_handle() const
	{
		return ns;
	}

	/// @return The module init function definition
	std::string get_def() const override;

	/// @return The registration to its parent
	std::string get_reg() const;

	/// Adds a nested module
	/// @param[in] m The nested module to add
	void add( Module&& m );

	/// Adds a function to this module
	/// @param[in] f The function to add to this module
	void add( Function&& f );

	/// Adds an enum to this module
	/// @param[in] e The enum to add to this module
	void add( Enum&& e );

	/// Adds a record to this module
	/// @param[in] r The record to add to this module
	void add( CXXRecord&& r );

	/// @return The methods map for the module init function
	const Methods& get_methods() const
	{
		return methods;
	}

	/// @return The nested modules within this module
	std::vector<Module>& get_children()
	{
		return modules;
	}

	/// @return The nested modules within this module
	const std::vector<Module>& get_modules() const
	{
		return modules;
	}

	/// @return Functions associated with the module
	const std::vector<Function>& get_functions() const
	{
		return functions;
	}

	/// @return Enums associated with the module
	const std::vector<Enum>& get_enums() const
	{
		return enums;
	}

	/// @return CXXRecords associated with the module
	const std::vector<CXXRecord>& get_records() const
	{
		return records;
	}

  protected:
	/// @return The Python name of the binding
	virtual void gen_py_name() override;

	/// @return A signature of the binding
	virtual void gen_sign() override;

	/// @return A definition of the bindings
	virtual void gen_def() override;

	/// @return Registration of this module
	void gen_reg();

  private:
	/// Namespace decl
	const clang::NamespaceDecl* ns;

	/// Python MethodDef
	Methods methods;

	/// Module registration
	std::stringstream reg;

	/// Module functions
	std::vector<Module> modules;

	/// Module functions
	std::vector<Function> functions;

	/// Module enums
	std::vector<Enum> enums;

	/// Module records
	std::vector<CXXRecord> records;
};

}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDINGS_MODULE_H_
