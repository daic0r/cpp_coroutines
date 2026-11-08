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

    template<typename T>
    struct data_slot {
      std::optional<T> data_;
    public:
      data_slot() = default;
      template<typename U = T>
      data_slot(U&& u) : data_{ std::forward<U>(u) } {}

      void set(T val) { data_ = std::move(val); }
      T take() { assert(data_); T ret = std::move(*data_); data_.reset(); return ret; }
      bool empty() const noexcept { return !data_.has_value(); }
    };

    template<>
    struct data_slot<void> {
    };

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
      const auto pPrev = current_coro;
      current_coro = this;
      switch_context(&caller_sp_, sp_);
      current_coro = pPrev;
    }

    void suspend() {
      switch_context(&sp_, caller_sp_);
    }

    void destroy() {
      delete[] stack_frame_;
      stack_frame_ = nullptr;
    }
  };


  template<typename Yield, typename Ret>
  class coroutine : public coroutine_base {
    detail::data_slot<Yield> yielded_;
    detail::data_slot<Ret> returned_;
    
  public:
    coroutine(std::function<Ret(coroutine&)> func)
      : coroutine_base([f=std::move(func), this](coroutine_base& c) {
	if constexpr (std::is_same_v<Ret, void>) {
	  f(static_cast<coroutine&>(c));
	} else {
	  returned_.set(f(static_cast<coroutine&>(c)));
	}
      })
    {
    }

    template<typename Y = Yield, typename = std::enable_if_t<!std::is_same_v<Y, void>>>
    Y get_value() {
      assert(!yielded_.empty());
      return yielded_.take();
    }

    template<typename R = Ret, typename = std::enable_if_t<!std::is_same_v<R, void>>>
    R get_return_value() {
      assert(!returned_.empty());
      return returned_.take();
    }

    void yield(Yield val) {
      yielded_ = std::move(val);
      suspend();
    }
  };

} // namespace

#endif
