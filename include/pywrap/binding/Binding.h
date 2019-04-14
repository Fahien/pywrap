#ifndef PYWRAP_BINDING_H_
#define PYWRAP_BINDING_H_

#include <sstream>

#include <clang/AST/Decl.h>

namespace pywrap
{
namespace binding
{
/// Contains declaration and definition of the Python bindings
/// of a C++ Decl (namespace, function, enum, class, etc.)
class Binding
{
  public:
	Binding( const clang::NamedDecl* n = nullptr, const Binding* parent = nullptr );

	virtual ~Binding() = default;

	Binding( Binding&& ) = default;

	/// @return An unique id
	const std::string& get_id() const
	{
		return id;
	}

	/// Initializes the members
	virtual void init();

	/// @return The relative path to the header
	const std::string& get_incl() const
	{
		return incl;
	}

	/// @return The original name
	std::string get_name() const
	{
		return name.str();
	}

	/// @return The python name
	std::string get_py_name() const
	{
		return py_name.str();
	}

	/// @return The signature of the binding
	std::string get_sign() const
	{
		return sign.str();
	}

	/// @return The declaration of the binding
	virtual std::string get_decl() const
	{
		return decl.str();
	}

	/// @return The definition of the binding
	virtual std::string get_def() const
	{
		return def.str();
	}

	/// Generates the relative path to the header
	void set_incl( const std::string& i )
	{
		incl = i;
	}

  protected:
	/// Generates the name of the binding
	virtual void gen_name();

	/// Generates the Python name of the binding
	virtual void gen_py_name();

	/// Generates the signature of the binding
	virtual void gen_sign(){};

	/// Generates the declaration of the bindings
	virtual void gen_decl();

	/// Generates the definition of the bindings
	virtual void gen_def(){};

	/// Named decl
	const clang::NamedDecl* named{ nullptr };

	/// Qualified name as unique id
	std::string id;

	/// Parent
	const Binding* parent{ nullptr };

	/// Relative path
	std::string incl;

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

}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDING_H_