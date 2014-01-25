#include "sequence_instruction.h"

namespace mfast
{
  sequence_field_instruction::sequence_field_instruction(uint16_t                          field_index,
                                                         presence_enum_t                   optional,
                                                         uint32_t                          id,
                                                         const char*                       name,
                                                         const char*                       ns,
                                                         const char*                       dictionary,
                                                         const const_instruction_ptr_t*    subinstructions,
                                                         uint32_t                          subinstructions_count,
                                                         const uint32_field_instruction*   sequence_length_instruction,
                                                         const char*                       typeref_name,
                                                         const char*                       typeref_ns,
                                                         const char*                       cpp_ns,
                                                         const group_field_instruction*    element_instruction,
                                                         const sequence_field_instruction* ref_inst)
    : group_field_instruction(field_index,
                              optional,
                              id,
                              name,
                              ns,
                              dictionary,
                              subinstructions,
                              subinstructions_count,
                              typeref_name,
                              typeref_ns,
                              cpp_ns)
    , sequence_length_instruction_(sequence_length_instruction)
    , element_instruction_(element_instruction)
  {
    field_type_ = field_type_sequence;
    if (ref_inst) {
      ref_instruction(ref_inst);
      element_instruction_=ref_inst->element_instruction();
    }
    if (element_instruction_) {
      this->set_subinstructions(element_instruction_->subinstructions(),
                                element_instruction_->subinstructions_count());
    }
  }

  void sequence_field_instruction::construct_sequence_elements(value_storage& storage,
                                                               std::size_t    start,
                                                               std::size_t    length,
                                                               allocator*     alloc) const
  {
    value_storage* elements = static_cast<value_storage*>(storage.of_array.content_);
    for (std::size_t i = start; i < start+length; ++i ) {
      construct_group_subfields(&elements[i*subinstructions_count_], alloc);
    }
  }

  void sequence_field_instruction::destruct_sequence_elements(value_storage& storage,
                                                              std::size_t    start,
                                                              std::size_t    length,
                                                              allocator*     alloc) const
  {
    value_storage* elements = static_cast<value_storage*>(storage.of_array.content_);
    for (std::size_t i = start; i < start+length; ++i ) {
      destruct_group_subfields(&elements[i*subinstructions_count_], alloc);
    }
  }

  void sequence_field_instruction::construct_value(value_storage& storage,
                                                   allocator*     alloc ) const
  {
    // len_ == 0 is reserve for null/absent
    uint32_t initial_length = sequence_length_instruction_->initial_value().get<uint32_t>();
    storage.of_array.len_ = optional() ? 0 : initial_length+1;
    if (sequence_length_instruction_ && initial_length > 0) {
      std::size_t element_size = this->group_content_byte_count();
      std::size_t reserve_size = initial_length*element_size;
      storage.of_array.content_ = 0;
      storage.of_array.capacity_in_bytes_ =  alloc->reallocate(storage.of_array.content_, 0, reserve_size);
      construct_sequence_elements(storage,0, storage.of_array.capacity_in_bytes_/element_size, alloc);
    }
    else {
      storage.of_array.content_ = 0;
      storage.of_array.capacity_in_bytes_ = 0;
    }
  }

  void sequence_field_instruction::destruct_value(value_storage& storage,
                                                  allocator*     alloc ) const
  {
    if (storage.of_array.capacity_in_bytes_) {
      destruct_sequence_elements(storage, 0, storage.of_array.capacity_in_bytes_/this->group_content_byte_count(), alloc);
      alloc->deallocate(storage.of_array.content_, storage.of_array.capacity_in_bytes_ );
    }
  }

  void sequence_field_instruction::construct_value(value_storage& storage,
                                                   value_storage*,
                                                   allocator*     alloc,
                                                   bool           ) const
  {
    this->construct_value(storage, alloc);
  }

  void sequence_field_instruction::copy_construct_value(const value_storage& src,
                                                        value_storage&       dest,
                                                        allocator*           alloc,
                                                        value_storage*) const
  {
    std::size_t size = src.of_array.len_;

    if (size > 0)
      --size;

    dest.of_array.len_ = src.of_array.len_;

    if (size) {
      std::size_t element_size = this->group_content_byte_count();
      std::size_t reserve_size = size*element_size;

      dest.of_array.content_ = 0;
      dest.of_array.capacity_in_bytes_ =  alloc->reallocate(dest.of_array.content_, 0, reserve_size);

      const value_storage* src_elements = static_cast<const value_storage*>(src.of_array.content_);
      value_storage* dest_elements = static_cast<value_storage*>(dest.of_array.content_);

      for (std::size_t i = 0; i < size; ++i ) {
        std::size_t j = i* this->subinstructions_count_;
        copy_group_subfields(&src_elements[j], &dest_elements[j], alloc);
      }
      // we must default construct the extra elements we reserved; otherwise we may deallocate garbage pointers during destruction.
      std::size_t unused_count = dest.of_array.capacity_in_bytes_/element_size - size;
      if (unused_count > 0) {
        // std::memset( static_cast<char*>(dest.of_array.content_) + reserve_size, 0, unused);
        construct_sequence_elements(dest, size, unused_count, alloc);
      }
    }
    else {
      dest.of_array.content_ = 0;
      dest.of_array.capacity_in_bytes_ = 0;
    }
  }

  sequence_field_instruction*
  sequence_field_instruction::clone(arena_allocator& alloc) const
  {
    return new (alloc) sequence_field_instruction(*this);
  }

} /* mfast */