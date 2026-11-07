#ifndef COROUTINE_HPP
#define COROUTINE_HPP

#include <iostream>
#include <cstring>
#include <cstdint>
#include <functional>

extern "C" void switch_context(void*, void*);


/*

define co_init(coroutine* pCoro)
     asm_init_coroutine(&pCoro->stack_frame);

void coro(coroutine* pCoro) {
   co_init(pCoro);
}

define run_coro()
    asm_run_coro(&pCoro->stack_frame);
  
void A() {
  coroutine c = run_coro(B());
}
*/
namespace ice {
  static constexpr std::size_t STACK_SIZE = 1024*16;

  struct coroutine {
    char *buffer_ptr;
    char *sp;
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
    current_coro = &coro;
    switch_context(&current_coro->caller_sp, current_coro->sp);
  }

  void co_suspend(coroutine& coro) {
    current_coro = &coro;
    switch_context(&current_coro->sp, current_coro->caller_sp);
  }

  coroutine make_coroutine(void(*func)(coroutine&)) {
    coroutine ret{};
    ret.sp = new char[STACK_SIZE];
    ret.buffer_ptr = ret.sp;
    ret.sp += STACK_SIZE;
    ret.func = func;
    
    std::ptrdiff_t aligned_top = reinterpret_cast<std::ptrdiff_t>(ret.sp + STACK_SIZE) & ~std::ptrdiff_t{15};
    
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
