#ifndef PYWRAP_BINDING_TAG_H_
#define PYWRAP_BINDING_TAG_H_

#include "pywrap/binding/Destructor.h"
#include "pywrap/binding/TypeObject.h"

namespace pywrap
{
namespace binding
{
class Tag : public Binding
{
  public:
	/// Generates bindings for a Tag (struct/union/class/enum)
	/// @param[in] Tag to wrap
	Tag( const clang::TagDecl* t );

	virtual ~Tag() = default;

	Tag( Tag&& ) = default;

	/// Initialized the tag
	void init() override;

	/// @return The qualified name
	const std::string& get_qualified_name() const { return qualified_name; }

	/// @return The destructor
	const Destructor& get_destructor() const { return destructor; }

	/// @return The type object
	const TypeObject& get_type_object() const { return type_object; }

	/// @return The declaration
	std::string get_decl() const override;

	/// @return The declaration
	std::string get_def() const override;

	/// @return The registration to the module
	std::string get_reg() const { return reg.str(); }

  protected:
	virtual void gen_reg(){};

	/// Module registration
	std::stringstream reg;

  private:
	/// Tag decl
	const clang::TagDecl* tag;

	/// Qualified name
	std::string qualified_name;

	/// Destructor
	Destructor destructor;

	/// Python type object
	TypeObject type_object;
};
}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDING_TAG_H_
