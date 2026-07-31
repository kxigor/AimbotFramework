#include <afw/hook/vtable.hpp>
#include <cstddef>

namespace afw::hook {

Vtable::Vtable(void** table) noexcept : table_{table} {}

void** Vtable::Get() const noexcept { return table_; }

void** Vtable::Slot(std::size_t index) const noexcept { return table_ + index; }

void* Vtable::At(std::size_t index) const noexcept { return *Slot(index); }

}  // namespace afw::hook
