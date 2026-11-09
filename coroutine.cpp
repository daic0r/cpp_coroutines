#include "coroutine.hpp"
#include <cstdint>

namespace ice {
  coroutine_base* current_coro;

  namespace detail {
    void co_invoke() {
      try {
	current_coro->func_(*current_coro);
      }
      catch (...) {
	current_coro->exception_ = std::current_exception();
      }
    }
    extern "C" void co_finish() {
      current_coro->done_ = true;
      switch_context(&current_coro->sp_, current_coro->caller_sp_);
      std::abort(); // should never be reached
    }
  }

  coroutine_base::coroutine_base(std::function<void(coroutine_base&)> func)
      : func_{ std::move(func) }
    {

      //stack_frame_ = new char[STACK_SIZE];
      // STACK_SIZE must be a multiple of the page size (e.g. 64 * 1024)
      const std::size_t guard = sysconf(_SC_PAGESIZE);
      allocated_size_ = guard + STACK_SIZE;

      void* mem = mmap(nullptr, allocated_size_, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
      if (mem == MAP_FAILED) throw std::bad_alloc{};
      mprotect(mem, guard, PROT_NONE);        // lowest page: stack grows down into it

      stack_frame_ = static_cast<char*>(mem);    
      std::ptrdiff_t aligned_top = reinterpret_cast<std::ptrdiff_t>(stack_frame_ + allocated_size_) & ~std::ptrdiff_t{15};
    
      void** sp = reinterpret_cast<void**>(aligned_top);
      *--sp = reinterpret_cast<void*>(&co_finish_trampoline);
      *--sp = reinterpret_cast<void*>(&detail::co_invoke);
    
      for (std::size_t i{}; i < 6; ++i)
	*--sp = nullptr;

      constexpr std::uint64_t initial_fp_state =
	(std::uint64_t{0x037F} << 32)   // x87 control word at offset 4
	| 0x1F80;                         // MXCSR at offset 0
      *--sp = reinterpret_cast<void*>(initial_fp_state);

      sp_ = reinterpret_cast<void*>(sp);

    }

}

