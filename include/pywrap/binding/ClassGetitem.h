#ifndef PYWRAP_BINDING_CLASSGETITEM_H_
#define PYWRAP_BINDING_CLASSGETITEM_H_

#include "pywrap/binding/Binding.h"

namespace pywrap
{
namespace binding
{
class Tag;
class Specialization;

class ClassGetitem : public Binding
{
  public:
	ClassGetitem( const Tag* t = nullptr ) : tag{ t }
	{
	}

	void add( const Specialization& s );

	virtual std::string get_def() const override;

  protected:
	virtual void gen_py_name() override;
	virtual void gen_sign() override;
	virtual void gen_def() override;

  private:
	const Tag* tag;
};


}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_BINDING_CLASSGETITEM_H_
