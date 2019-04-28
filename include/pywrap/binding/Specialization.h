#ifndef PYWRAP_SPECIALIZATION_H_
#define PYWRAP_SPECIALIZATION_H_

#include "pywrap/binding/CXXRecord.h"

#include <clang/AST/DeclTemplate.h>

namespace pywrap
{
namespace binding
{
class Specialization : public CXXRecord
{
  public:
	Specialization( const clang::ClassTemplateSpecializationDecl* spec, const Binding& parent );

	virtual void gen_name() override;

  private:
	// TODO Metaclass __getitem__ type object

	const clang::ClassTemplateSpecializationDecl* spec;
};

}  // namespace binding
}  // namespace pywrap

#endif  // PYWRAP_SPECIALIZATION_H_
