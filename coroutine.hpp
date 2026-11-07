#ifndef COROUTINE_HPP
#define COROUTINE_HPP

#include <iostream>
#include <functional>
#include <cassert>
#include <optional>

extern "C" void switch_context(void** save_rsp, void* load_rsp);
extern "C" void co_finish_trampoline();

namespace ice {
  static constexpr std::size_t STACK_SIZE = 1024*16;

  /*
  struct coroutine {
    char *buffer_ptr;
    void *sp;
    void *caller_sp;
    std::function<void(coroutine&)> func;
    bool done = false;
  };
  */

  class coroutine_base;

  extern coroutine_base* current_coro;

  namespace detail {
    void co_invoke();
    extern "C" void co_finish();
  }
  

  class coroutine_base {
    friend void detail::co_finish();
    friend void detail::co_invoke();
    
    char *stack_frame_{};
    void *sp_{};
    void *caller_sp_{};
    std::function<void(coroutine_base&)> func_{};
    bool done_{};
    
  protected:
    coroutine_base(std::function<void(coroutine_base&)> func);

  public:
    virtual ~coroutine_base() {
      if (stack_frame_) {
	destroy();
      }
    }

    bool done() const noexcept {
      return done_;
    }

    void resume() {
      assert(!done());
      current_coro = this;
      switch_context(&caller_sp_, sp_);
    }

    void suspend() {
      current_coro = this;
      switch_context(&sp_, caller_sp_);
    }

    void destroy() {
      delete[] stack_frame_;
      stack_frame_ = nullptr;
    }

    
    
  };

  template<typename Ret>
  class coroutine : public coroutine_base {
    std::optional<Ret> value_;
    
  public:
    coroutine(std::function<void(coroutine&)> func)
      : coroutine_base([f=std::move(func)](coroutine_base& c) { f(static_cast<coroutine&>(c)); })
    {
      resume();
    }

    Ret get_value() {
      assert(value_.has_value());
      return std::move(*value_);
    }

    void yield(Ret val) {
      value_ = std::move(val);
      suspend();
    }
  };


  template<>
  class coroutine<void> : public coroutine_base {
  public:
    coroutine(std::function<void(coroutine&)> func)
      : coroutine_base([f=std::move(func)](coroutine_base& c) { f(static_cast<coroutine&>(c)); })
    {}
  };


} // namespace

#endif
