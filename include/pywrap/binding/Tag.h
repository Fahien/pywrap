#ifndef PYWRAP_BINDING_TAG_H_
#define PYWRAP_BINDING_TAG_H_

#include <clang/AST/DeclTemplate.h>

#include "pywrap/binding/ClassGetitem.h"
#include "pywrap/binding/Compare.h"
#include "pywrap/binding/Destructor.h"
#include "pywrap/binding/Init.h"
#include "pywrap/binding/Method.h"
#include "pywrap/binding/TypeObject.h"
#include "pywrap/binding/Wrapper.h"

namespace pywrap
{
namespace binding
{
class Tag : public Binding
{
  public:
	/// This represents a PyMethodDef structure
	/// responsible to hold the methods associated to a Tag
	class Methods : public Binding
	{
	  public:
		/// @param[in] t Tag these methods belong to
		Methods( const Tag* t = nullptr );

		Methods( Methods&& ) = default;

		/// Adds a method to the methods map
		/// @param m The method to add
		void add( const Method& m );

		/// @return The declaration of the methods map
		virtual std::string get_decl() const override;

		/// @return The definition of the methods map
		virtual std::string get_def() const override;

	  protected:
		/// Generates the python name of the methods map
		void gen_py_name() override;

		/// Generates the signature
		void gen_sign() override;

		/// Generates the definition of the methods map
		void gen_def() override;

	  private:
		const Tag* tag;

		size_t size = 1;
	};

	/// This represents a PyMemberDef structure
	class Members : public Binding
	{
	  public:
		/// @param[in] t Tag these methods belong to
		Members( const Tag* t = nullptr );

		Members( Members&& ) = default;

		/// @return The declaration of the member map
		std::string get_decl() const override;

	  protected:
		/// Generates the python name of the members map
		void gen_py_name() override;

		/// Generates the signature
		void gen_sign() override;

		/// Generates the definition of the members map
		void gen_def() override;

	  private:
		const Tag* tag;

		size_t size = 1;
	};

	/// Represents a getset map
	class Accessors : public Binding
	{
	  public:
		/// @param[in] t Tag these methods belong to
		Accessors( const Tag* t = nullptr );

		Accessors( Accessors&& ) = default;

		/// @return The declaration of the getset map
		std::string get_decl() const override;

	  protected:
		/// Generates the python name of the getset map
		void gen_py_name() override;

		/// Generates the signature
		void gen_sign() override;

		/// Generates the definition of the getset map
		void gen_def() override;

	  private:
		const Tag* tag;

		size_t size = 1;
	};

	/// Generates bindings for a Tag (struct/union/class/enum)
	/// @param[in] t Tag to wrap
	/// @param[in] p Parent of the tag
	Tag( const clang::TagDecl& t, const Binding& p );

	/// Generates bindings for a template class
	/// @param[in] t Template class
	/// @param[in] p Parent of the tag
	Tag( const clang::ClassTemplateDecl& t, const Binding& p );

	virtual ~Tag() = default;

	Tag( Tag&& ) = default;

	/// Initialized the tag
	void init() override;

	/// @return The tag decl
	const clang::TagDecl* operator->() const
	{
		return tag;
	}

	/// @return The tag decl
	const clang::TagDecl* get_handle() const
	{
		return tag;
	}

	/// @return The template decl
	const clang::ClassTemplateDecl* get_templ() const
	{
		return templ;
	}

	/// @return The destructor
	const Destructor& get_destructor() const
	{
		return destructor;
	}

	/// @return The initializer
	const Init& get_init() const
	{
		return initializer;
	}

	/// @return The compare func
	const Compare& get_compare() const
	{
		return compare;
	}

	/// @return The __class_getitem__ func
	const ClassGetitem& get_class_getitem() const
	{
		return class_getitem;
	}

	/// @return The methods map
	const Methods& get_methods() const
	{
		return methods;
	}

	/// @return The members map
	const Members& get_members() const
	{
		return members;
	}

	/// @return The getset map
	const Accessors& get_accessors() const
	{
		return accessors;
	}

	/// @return The type object
	const TypeObject& get_type_object() const
	{
		return type_object;
	}

	/// @return The wrapper specialization
	const Wrapper& get_wrapper() const
	{
		return wrapper;
	}

	/// @return The declaration
	virtual std::string get_decl() const override;

	/// @return The definition
	virtual std::string get_def() const override;

	/// @return The registration to the module
	std::string get_reg() const
	{
		return reg.str();
	}

  protected:
	virtual void gen_fields()
	{
	}

	virtual void gen_reg();

	/// @return The __class_getitem__ func
	ClassGetitem& get_mut_class_getitem()
	{
		return class_getitem;
	}

	/// Module registration
	std::stringstream reg;

  private:
	/// Tag decl
	const clang::TagDecl* tag = nullptr;

	/// Template decl
	const clang::ClassTemplateDecl* templ = nullptr;

	/// Destructor
	Destructor destructor;

	/// Init
	Init initializer;

	/// Compare
	Compare compare;

	/// ClassGetitem
	ClassGetitem class_getitem;

	/// Methods
	Methods methods;

	/// Members
	Members members;

	/// Accessors
	Accessors accessors;

	/// Python type object
	TypeObject type_object;

	/// Wrapper template specialization
	Wrapper wrapper;
};

}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDING_TAG_H_
