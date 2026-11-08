#include <iostream>
#include <cstdint>
#include <cstring>
#include "coroutine.hpp"
#include <string>

/*
int f(int a, int b) {
  volatile void *ptr;
  std::ptrdiff_t rbp, rsp;
  asm volatile( "mov %%rsp, %0"
		: "=r" (rsp)
		:
		:
		);
  asm volatile( "mov %%rbp, %0"
		: "=r" (rbp)
		:
		:
		);
  std::cout << "rbp = " << rbp << "\n";
  std::cout << "rsp = " << rsp << std::endl;

  std::ptrdiff_t prev_rbp;
  std::memcpy(&prev_rbp, reinterpret_cast<std::ptrdiff_t*>(rbp), sizeof(std::ptrdiff_t));

  std::cout << "prev_rbp = " << prev_rbp << "\n";


  return a + b;
}

__attribute__((noinline)) void test_func(ice::coroutine* coro) {
  int i = 4;
  static_assert(sizeof(int) == 4, "wrong size for int");
  std::cout << "i sitting at " << std::dec << (std::ptrdiff_t) &i << "\n";

  co_yield();
  
  std::cout << "Coroutine resumed\n";
}

int main() {

  std::ptrdiff_t rbp, rsp;
  asm volatile( "mov %%rsp, %0"
		: "=r" (rsp)
		:
		:
		);
  asm volatile( "mov %%rbp, %0"
		: "=r" (rbp)
		:
		:
		);
  std::cout << std::hex << "rbp = " << rbp << "\n";
  std::cout << "rsp = " << rsp << std::endl;

  co_init();
  std::cout << std::dec << "Stack frame at " << (std::ptrdiff_t)coro.stack_frame << "\n";
  test_func(&coro);

  std::cout << "Coroutine suspended\n";
  
  return 0;
}
*/

std::string func(ice::coroutine<int, std::string>& coro) {
  std::cout << "Entered coroutine\n";
  coro.yield(23);
  std::cout << "Resumed coroutine!\n";
  return "Test return";
}

int main() {
  
  auto coro = ice::coroutine<int, std::string>(func);
  /*
ice::current_coro = &coro;
  ice::co_resume(coro);
  std::cout << "Coroutine suspended, we can do more work in main\n";
  ice::co_resume(coro);
  ice::co_destroy(coro);
  std::cout << "Coroutine finished execution\n";
  */
  coro.resume();
  std::cout << "Coroutine suspended, we can do more work in main\n";
  std::cout << "Coroutine generated value: " << coro.get_value() << "\n";
  coro.resume();
  if (coro.done()) {
    std::cout << "Coroutine returned " << coro.get_return_value() << "\n";
  }
  std::cout << "Coroutine finished execution\n";

  return 0;
}
