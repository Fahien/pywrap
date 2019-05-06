#ifndef PYWRAP_BINDING_CXXRECORD_H_
#define PYWRAP_BINDING_CXXRECORD_H_

#include "pywrap/binding/Field.h"
#include "pywrap/binding/Tag.h"

namespace pywrap
{
namespace binding
{
/// Represents the Python bindings of a struct/union/class
class CXXRecord : public Tag
{
  public:
	/// Generates definition and declaration of Python bindings for a struct/union/class
	/// @param[in] rec CXXRecord to wrap
	CXXRecord( const clang::CXXRecordDecl& rec, const Binding& parent );

	virtual void gen_fields() override;

	const std::vector<Field>& get_fields() const
	{
		return fields;
	}

	void add_field( Field&& f );

	virtual void init() override;

	virtual std::string get_decl() const override;

	virtual std::string get_def() const override;

  private:
	/// CXXRecord decl
	const clang::CXXRecordDecl& record;

	std::vector<Field> fields;
};


class Specialization : public CXXRecord
{
  public:
	Specialization( const clang::ClassTemplateSpecializationDecl& spec, const Binding& parent );

	void gen_template_args();

	virtual void gen_name() override;

	virtual void gen_qualified_name() override;

	virtual void gen_fields() override;

	const clang::ArrayRef<clang::TemplateArgument>& get_args() const
	{
		return args;
	}

	const clang::TemplateArgument* get_arg( const clang::FieldDecl& f ) const;

  private:
	const clang::ClassTemplateSpecializationDecl& spec;

	clang::ArrayRef<clang::TemplateArgument> args;

	std::stringstream template_args;
};


class Template : public Tag
{
  public:
	Template( const clang::ClassTemplateDecl& templ, const Binding& parent );

	const std::vector<const Specialization*> get_specializations() const
	{
		return specializations;
	}

	/// Adds a specialization to the template
	/// @param[in] spec Template specialization to add
	void add( const Specialization& spec );

  private:
	const clang::ClassTemplateDecl& templ;

	std::vector<const Specialization*> specializations;
};

}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDING_CXXRECORD_H_
