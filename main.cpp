#include <iostream>
#include <cstdint>
#include <cstring>

extern "C" void switch_context();

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
  std::cout << "rbp = " << rbp << "\n";
  std::cout << "rsp = " << rsp << std::endl;

  switch_context();
  
  return f(5, 4);
}
