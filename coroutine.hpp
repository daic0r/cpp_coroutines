#ifndef COROUTINE_HPP
#define COROUTINE_HPP

#include <exception>
#include <new>
#include <cstdlib>
#include <functional>
#include <cassert>
#include <optional>
#include <utility>
#include <unistd.h>
#include <sys/mman.h>

extern "C" void switch_context(void** save_rsp, void* load_rsp);
extern "C" void co_finish_trampoline();

namespace ice {
  static constexpr std::size_t STACK_SIZE = 1024*256;

  class coroutine_base;

  extern coroutine_base* current_coro;

  namespace detail {
    void co_invoke();
    extern "C" void co_finish();

    template<typename T>
    struct type_identity {
      using type = T;
    };

    struct coroutine_canceled_signal {};
  }

  enum class coroutine_state {
    NOT_STARTED,
    RUNNING,
    SUSPENDED,
    CANCELED,
    DONE
  };
  

  class coroutine_base {
    friend void detail::co_finish();
    friend void detail::co_invoke();
    
    char *stack_frame_{};
    void *sp_{};
    void *caller_sp_{};
    std::function<void(coroutine_base&)> func_{};
    std::exception_ptr exception_{};
    std::size_t allocated_size_{};
    coroutine_state state_{ coroutine_state::NOT_STARTED };

  private:
    void destroy() {
      munmap(stack_frame_, allocated_size_);
      stack_frame_ = nullptr;
    }
    
  protected:
    coroutine_base(std::function<void(coroutine_base&)> func);
    coroutine_base(const coroutine_base&) = delete;
    coroutine_base& operator=(const coroutine_base&) = delete;
    coroutine_base(coroutine_base&& rhs) noexcept = delete;
    coroutine_base& operator=(coroutine_base&& rhs) noexcept = delete;

  public:
    
    virtual ~coroutine_base() {
      assert(state_ == coroutine_state::DONE);
      destroy();
    }

    bool done() const noexcept {
      return state_ == coroutine_state::DONE;
    }

    coroutine_state state() const noexcept {
      return state_;
    }

    void cancel() {
      if (state_ == coroutine_state::DONE || stack_frame_ == nullptr)
	return;
      state_ = coroutine_state::CANCELED;
      try {
	// cancel() will be called from the dtor, so we catch
	// all exceptions here in order not to throw out of the
	// dtor
	resume();
      }
      catch (...) {}
    }

    void resume() {
      assert(state_ != coroutine_state::DONE);
      const auto pPrev = current_coro;
      current_coro = this;
      if (state_ != coroutine_state::CANCELED) {
	state_ = coroutine_state::RUNNING;
      }
      switch_context(&caller_sp_, sp_);
      current_coro = pPrev;
      
      if (exception_)
	std::rethrow_exception(std::exchange(exception_, nullptr));
    }

    void suspend() {
      if (state_ != coroutine_state::CANCELED)
	state_ = coroutine_state::SUSPENDED;
      switch_context(&sp_, caller_sp_);
      if (state_ == coroutine_state::CANCELED) {
	throw detail::coroutine_canceled_signal{};
      }
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
      : coroutine_data_layer<Ret, Yield>([f=std::move(func), this](coroutine_base& c) mutable {
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

    ~coroutine() {
      assert(this->state() != coroutine_state::RUNNING);
      this->cancel();
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

    template<typename Y = Yield, typename = std::enable_if_t<!std::is_void_v<Y>>>
    void yield(typename detail::type_identity<Y>::type val) {
      this->yielded_ = std::move(val);
      this->suspend();
    }

    template<typename Y = Yield,  typename = std::enable_if_t<!std::is_same_v<Y, void>>>
    Y next() {
      this->resume();
      return get_value();
    }

    template<typename Awaitable>
    auto await(Awaitable a) -> typename std::invoke_result_t<decltype(&Awaitable::get_result), Awaitable*>
    {
      if (a.result_available())
	return a.get_result();
      a.will_suspend();
      this->suspend();
      return a.get_result();
    }
  };

} // namespace

#endif
