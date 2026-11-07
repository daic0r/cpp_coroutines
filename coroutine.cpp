#include "coroutine.hpp"

namespace ice {
  coroutine_base* current_coro;

  namespace detail {
    void co_invoke() {
      current_coro->func_(*current_coro);
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

      stack_frame_ = new char[STACK_SIZE];
      sp_ = stack_frame_ + STACK_SIZE;
    
      std::ptrdiff_t aligned_top = reinterpret_cast<std::ptrdiff_t>(static_cast<char*>(sp_) + STACK_SIZE) & ~std::ptrdiff_t{15};
    
      void** sp = reinterpret_cast<void**>(aligned_top);
      *--sp = reinterpret_cast<void*>(&co_finish_trampoline);
      *--sp = reinterpret_cast<void*>(&detail::co_invoke);
    
      for (std::size_t i{}; i < 6; ++i)
	*--sp = nullptr;

      sp_ = reinterpret_cast<void*>(sp);

    }

}

