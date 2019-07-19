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
	Module( const clang::NamedDecl& n, const Binding* parent = nullptr );

	Module( Module&& ) = default;

	const clang::NamedDecl* get_handle() const
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
	/// @param[in] f The function to add
	void add( Function&& f );

	/// Adds an enum to this module
	/// @param[in] e The enum to add
	void add( Enum&& e );

	/// Adds a record to this module
	/// @param[in] r The record to add
	void add( CXXRecord&& r );

	/// Adds a template to this module
	/// @param[in] t The template to add
	void add( Template&& t );

	/// Adds a template specialization to this module
	/// @param[in] t The template specialization to add
	void add( Specialization&& s );

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

	/// @return Templates associated with the module
	const std::vector<Template>& get_templates() const
	{
		return templates;
	}

	/// @return Specializations associated with the module
	const std::vector<Specialization>& get_specializations() const
	{
		return specializations;
	}

  protected:
	/// @return A signature of the binding
	virtual void gen_sign() override;

	/// @return A definition of the bindings
	virtual void gen_def() override;

	/// @return Registration of this module
	void gen_reg();

  private:
	/// Named decl
	const clang::NamedDecl* ns;

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

	/// Module templates
	std::vector<Template> templates;

	/// Module templates
	std::vector<Specialization> specializations;
};

}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDINGS_MODULE_H_
