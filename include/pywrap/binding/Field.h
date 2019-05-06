#ifndef PYWRAP_BINDING_FIELD_H_
#define PYWRAP_BINDING_FIELD_H_

#include "pywrap/binding/Binding.h"

namespace pywrap
{
namespace binding
{
class Tag;
class Field;


class Getter : public Binding
{
  public:
	Getter( const Field& f ) : field{ &f }
	{
	}

  protected:
	virtual void gen_name() override;
	virtual void gen_sign() override;
	virtual void gen_def() override;

  private:
	const Field* field;

	friend Field;
};


class Setter : public Binding
{
  public:
	Setter( const Field& f ) : field{ &f }
	{
	}

	virtual void gen_name() override;
	virtual void gen_sign() override;
	virtual void gen_def() override;

  private:
	const Field* field;

	friend Field;
};


class Field
{
  public:
	Field( const clang::FieldDecl& f, const Tag& t );

	Field( Field&& );

	void init()
	{
		getter.init();
		setter.init();
	}

	const clang::FieldDecl& get_handle() const
	{
		return field;
	}

	const clang::QualType get_type() const;

	const Tag& get_tag() const
	{
		return tag;
	}

	const std::string& get_name() const
	{
		return name;
	}

	const Getter& get_getter() const
	{
		return getter;
	}
	const Setter& get_setter() const
	{
		return setter;
	}

  private:
	const clang::FieldDecl& field;

	const Tag& tag;

	std::string name;

	Getter getter;

	Setter setter;
};

}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDING_FIELD_H_
