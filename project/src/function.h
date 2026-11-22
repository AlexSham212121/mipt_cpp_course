#ifndef NANO_EDR_FUNCTION_H
#define NANO_EDR_FUNCTION_H

#include <memory>
#include <type_traits>
#include <utility>

namespace nano_edr {

template <typename Signature>
class Function;

template <typename R, typename... Args>
class Function<R(Args...)> {
 public:
    Function() = default;

    template <typename F>
    Function(F f) : impl_(std::make_unique<Holder<F>>(std::move(f))) {}

    Function(const Function&) = delete;
    Function& operator=(const Function&) = delete;
    Function(Function&&) noexcept = default;
    Function& operator=(Function&&) noexcept = default;

    R operator()(Args... args) const {
        return impl_->Call(args...);
    }

    explicit operator bool() const { return impl_ != nullptr; }

 private:
    struct Base {
        virtual ~Base() = default;
        virtual R Call(Args... args) const = 0;
    };

    template <typename F>
    struct Holder : Base {
        explicit Holder(F f) : fn(std::move(f)) {}

        R Call(Args... args) const override {
            return fn(args...);
        }

        mutable F fn;
    };

    std::unique_ptr<Base> impl_;
};

}

#endif