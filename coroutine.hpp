#ifndef COROUTINE_HPP
#define COROUTINE_HPP

#include <iostream>
#include <functional>
#include <cassert>
#include <optional>
#include <utility>

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
    coroutine_base(const coroutine_base&) = delete;
    coroutine_base& operator=(const coroutine_base&) = delete;
    coroutine_base(coroutine_base&& rhs) noexcept {
      stack_frame_ = std::exchange(rhs.stack_frame_, nullptr);
      sp_ = std::exchange(rhs.sp_, nullptr);
      caller_sp_ = std::exchange(rhs.caller_sp_, nullptr);
      func_ = std::exchange(rhs.func_, nullptr);
      done_ = std::exchange(rhs.done_, true);
    }
    coroutine_base& operator=(coroutine_base&& rhs) noexcept {
      coroutine_base copy{ std::move(rhs) };
      copy.swap(*this);
      return *this;
    }
    

  public:
    void swap(coroutine_base& rhs) noexcept {
      using std::swap;
      swap(stack_frame_, rhs.stack_frame_);
      swap(sp_, rhs.sp_);
      swap(caller_sp_, rhs.caller_sp_);
      swap(func_, rhs.func_);
      swap(done_, rhs.done_);

    }

    friend void swap(coroutine_base& lhs, coroutine_base& rhs) noexcept {
      lhs.swap(rhs);
    }
    
    virtual ~coroutine_base() {
      destroy();
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
      assert(done());
      delete[] stack_frame_;
      stack_frame_ = nullptr;
    }
  };

  template<typename Ret, typename Yield>
  class coroutine_data_layer : public coroutine_base {
  protected:
    using coroutine_base::coroutine_base;
    std::optional<Yield> yielded_;
    std::optional<Ret> returned_;
  };

  template<>
  class coroutine_data_layer<void, void> : public coroutine_base {
    using coroutine_base::coroutine_base;
    
  };

  template<typename Ret>
  class coroutine_data_layer<Ret, void> : public coroutine_base {
  protected:
    using coroutine_base::coroutine_base;
    
    std::optional<Ret> returned_;
  };


  template<typename Yield>
  class coroutine_data_layer<void, Yield> : public coroutine_base {
  protected:
    using coroutine_base::coroutine_base;
    
    std::optional<Yield> yielded_;
  };

  template<typename Ret, typename Yield = void>
  class coroutine : public coroutine_data_layer<Ret, Yield> {

    template<typename F, typename R = Ret>
    auto run(coroutine_base& c, F f) -> typename std::enable_if_t<!std::is_same_v<R, void>> {
      this->returned_ = f(static_cast<coroutine&>(c));
    }

    template<typename F, typename R = Ret>
    auto run(coroutine_base& c, F f) -> typename std::enable_if_t<std::is_same_v<R, void>> {
      f(static_cast<coroutine&>(c));
    }

  public:
    coroutine(std::function<Ret(coroutine&)> func)
      : coroutine_data_layer<Ret, Yield>([f=std::move(func), this](coroutine_base& c) {
	/*p
	if constexpr (std::is_same_v<Ret, void>) {
	  f(static_cast<coroutine&>(c));
	} else {
	  returned_ = f(static_cast<coroutine&>(c));
	}
	*/
	run(c, std::move(f));
      })
    {
    }

    template<typename Y = Yield, typename = std::enable_if_t<!std::is_same_v<Y, void>>>
    Y get_value() {
      assert(this->yielded_.has_value());
      return std::move(*this->yielded_);
    }

    template<typename R = Ret, typename = std::enable_if_t<!std::is_same_v<R, void>>>
    R get_return_value() {
      assert(this->returned_.has_value());
      return std::move(*this->returned_);
    }

    template<typename Y = Yield, typename = std::enable_if_t<!std::is_same_v<Y, void>>>
    void yield(Y val) {
      this->yielded_ = std::move(val);
      this->suspend();
    }

    template<typename Awaitable>
    auto await(Awaitable a) -> typename std::invoke_result_t<decltype(&Awaitable::get_result), Awaitable*>
    {
      if (a.result_available())
	return a.get_result();
      a.will_suspend();
      this->suspend();
      //      assert(a.result_available());
      return a.get_result();
    }
  };

} // namespace

#endif
