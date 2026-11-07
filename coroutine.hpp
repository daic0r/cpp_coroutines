#ifndef COROUTINE_HPP
#define COROUTINE_HPP

#include <iostream>
#include <functional>
#include <cassert>

extern "C" void switch_context(void** save_rsp, void* load_rsp);

namespace ice {
  static constexpr std::size_t STACK_SIZE = 1024*16;

  struct coroutine {
    char *buffer_ptr;
    void *sp;
    void *caller_sp;
    void (*func)(coroutine&);
    bool done = false;
  };

  coroutine* current_coro{};

  void co_invoke() {
    current_coro->func(*current_coro);
    current_coro->done = true;
    switch_context(&current_coro->sp, current_coro->caller_sp);
    std::abort(); // should never be reached
  }

  void co_resume(coroutine& coro) {
    assert(!coro.done);
    current_coro = &coro;
    switch_context(&current_coro->caller_sp, current_coro->sp);
  }

  void co_suspend(coroutine& coro) {
    current_coro = &coro;
    switch_context(&current_coro->sp, current_coro->caller_sp);
  }

  coroutine make_coroutine(void(*func)(coroutine&)) {
    coroutine ret{};
    ret.buffer_ptr = new char[STACK_SIZE];
    ret.sp = ret.buffer_ptr + STACK_SIZE;
    ret.func = func;
    
    std::ptrdiff_t aligned_top = reinterpret_cast<std::ptrdiff_t>(static_cast<char*>(ret.sp) + STACK_SIZE) & ~std::ptrdiff_t{15};
    
    void** sp = reinterpret_cast<void**>(aligned_top);
    *--sp = nullptr;
    *--sp = reinterpret_cast<void*>(&co_invoke);
    
    for (std::size_t i{}; i < 6; ++i)
      *--sp = nullptr;

    ret.sp = reinterpret_cast<char*>(sp);
    
    return ret;
  }

  void co_destroy(coroutine& coro) {
    delete[] coro.buffer_ptr;
  }
}

#endif
