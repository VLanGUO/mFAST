#include <string>
#include <map>
#include <set>
#include "mfast/field_instruction.h"
#include "../../../../tinyxml2/tinyxml2.h"
#include "../common/exceptions.h"
#include "field_builder.h"
#include "../dynamic_templates_description.h"
#include <boost/utility.hpp>


using namespace tinyxml2;


//////////////////////////////////////////////////////////////////////////////////////
namespace mfast
{
namespace coder
{


class templates_builder
  : public XMLVisitor
  , public boost::base_from_member<type_map_t>
  , public field_builder_base
{
public:
  templates_builder(templates_description* definition,
                    const char*            cpp_ns,
                    template_registry*     registry);

  virtual bool  VisitEnter (const XMLElement & element, const XMLAttribute* attr);
  virtual bool  VisitExit (const XMLElement & element);


  virtual std::size_t num_instructions() const;
  virtual void add_instruction(const field_instruction*);
  void add_template(const char* ns, template_instruction* inst);

  virtual const char* name() const
  {
    return cpp_ns_;
  }

protected:
  templates_description* definition_;
  const char* cpp_ns_;
  std::deque<const template_instruction*> templates_;
  const template_instruction template_instruction_prototype_;
};


templates_builder::templates_builder(templates_description* definition,
                                     const char*            cpp_ns,
                                     template_registry*     registry)
  : field_builder_base(registry->impl_, &this->member)
  , definition_(definition)
  , cpp_ns_(string_dup(cpp_ns, this->alloc()))
  , template_instruction_prototype_(0,0,"","","",0,0,0,0,0,cpp_ns_)
{
  static const int32_field_instruction int32_field_instruction_prototype(0,operator_none,presence_mandatory,0,0,"",0);
  this->member["int32"] = &int32_field_instruction_prototype;

  static const uint32_field_instruction uint32_field_instruction_prototype(0,operator_none,presence_mandatory,0,0,"",0);
  this->member["uInt32"] = &uint32_field_instruction_prototype;

  static const int64_field_instruction int64_field_instruction_prototype(0,operator_none,presence_mandatory,0,0,"",0);
  this->member["int64"] = &int64_field_instruction_prototype;

  static const uint64_field_instruction uint64_field_instruction_prototype(0,operator_none,presence_mandatory,0,0,"",0);
  this->member["uInt64"] = &uint64_field_instruction_prototype;

  static const decimal_field_instruction decimal_field_instruction_prototype(0,operator_none,presence_mandatory,0,0,"",0);
  this->member["decimal"] = &decimal_field_instruction_prototype;

  static const ascii_field_instruction ascii_field_instruction_prototype(0,operator_none,presence_mandatory,0,0,"",0);
  this->member["string"] = &ascii_field_instruction_prototype;

  static const byte_vector_field_instruction byte_vector_field_instruction_prototype(0,operator_none,presence_mandatory,0,0,"",0);
  this->member["byteVector"] = &byte_vector_field_instruction_prototype;

  static const int32_vector_field_instruction int32_vector_field_instruction_prototype(0,presence_mandatory,0,0,"");
  this->member["int32Vector"] = &int32_vector_field_instruction_prototype;

  static const uint32_vector_field_instruction uint32_vector_field_instruction_prototype(0,presence_mandatory,0,0,"");
  this->member["uInt32Vector"] = &uint32_vector_field_instruction_prototype;

  static const int64_vector_field_instruction int64_vector_field_instruction_prototype(0,presence_mandatory,0,0,"");
  this->member["int64Vector"] = &int64_vector_field_instruction_prototype;

  static const uint64_vector_field_instruction uint64_vector_field_instruction_prototype(0,presence_mandatory,0,0,"");
  this->member["uInt64Vector"] = &uint64_vector_field_instruction_prototype;

  static const group_field_instruction group_field_instruction_prototype(0,presence_mandatory,0,0,"","",0,0);
  this->member["group"] = &group_field_instruction_prototype;

  static const uint32_field_instruction length_instruction_prototype(0,operator_none,presence_mandatory,0,"__length__","",0);
  static const sequence_field_instruction sequence_field_instruction_prototype(0,presence_mandatory,0,0,"","",0,0,&length_instruction_prototype);
  this->member["sequence"] = &sequence_field_instruction_prototype;

  this->member["template"] = &template_instruction_prototype_;

  static const templateref_instruction templateref_instruction_prototype(0,presence_mandatory);
  this->member["templateRef"] = &templateref_instruction_prototype;
}

bool
templates_builder::VisitEnter (const XMLElement & element, const XMLAttribute*)
{
  const char* element_name = element.Name();

  if (std::strcmp(element_name, "templates") == 0 ) {
    definition_->ns_ = string_dup(get_optional_attr(element, "ns", ""), alloc());

    resolved_ns_= string_dup(get_optional_attr(element, "templateNs", ""), alloc());
    definition_->template_ns_ = resolved_ns_;
    definition_->dictionary_ = string_dup(get_optional_attr(element, "dictionary", ""), alloc());
    return true;
  }
  else if (strcmp(element_name, "define") == 0) {
    const char* name =  get_optional_attr(element, "name", 0);
    const XMLElement* elem = element.FirstChildElement();
    if (name && elem)
      field_builder(this, *elem, name);
  }
  else if (strcmp(element_name, "template") == 0) {
    field_builder(this, element);
  }
  return false;
}

bool
templates_builder::VisitExit (const XMLElement & element)
{
  if (std::strcmp(element.Name(), "templates") == 0 ) {
    typedef const template_instruction* const_template_instruction_ptr_t;

    definition_->instructions_ = new (alloc())const_template_instruction_ptr_t[this->num_instructions()];
    std::copy(templates_.begin(), templates_.end(), definition_->instructions_);
    definition_->instructions_count_ = templates_.size();
  }
  return true;
}

std::size_t templates_builder::num_instructions() const
{
  return registered_types()->size();
}

void templates_builder::add_instruction(const field_instruction* inst)
{
  member[inst->name()] = inst;
}

void templates_builder::add_template(const char* ns, template_instruction* inst)
{
  templates_.push_back(inst);
  registered_templates()->add(ns, inst);
}

/////////////////////////////////////////////////////////////////////////////////
}   /* coder */


template_registry::template_registry()
  : impl_(new coder::template_registry_impl)
{
}

template_registry::~template_registry()
{
  delete impl_;
}

template_registry*
template_registry::instance()
{
  static template_registry inst;
  return &inst;
}

arena_allocator* template_registry::alloc()
{
  return &(this->impl_->alloc_);
}

dynamic_templates_description::dynamic_templates_description(const char*        xml_content,
                                                             const char*        cpp_ns,
                                                             template_registry* registry)
{
  XMLDocument document;
  if (document.Parse(xml_content) == 0) {
    coder::templates_builder builder(this, cpp_ns, registry);
    document.Accept(&builder);
  }
  else {
    BOOST_THROW_EXCEPTION(fast_static_error("S1"));
  }
}

} /* mfast */
