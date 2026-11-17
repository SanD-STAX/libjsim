/// @file math.hpp
/// @defgroup math jsim-math 数学函数包
/// @brief java.lang.Math 的纯函数接管（doc:ext-jdk-math）。

#pragma once

#include <cmath>
#include <limits>

#include "jsim/engine.hpp"

namespace jsim::math {
namespace detail {

template <typename T> struct Number;

template <> struct Number<i32> {
    static constexpr ValType type = ValType::I32;
    static i32 read(const SimValue& v) { return static_cast<i32>(v.intBits); }
    static SimValue write(i32 v) { return SimValue::knownI32(v); }
};
template <> struct Number<i64> {
    static constexpr ValType type = ValType::I64;
    static i64 read(const SimValue& v) { return v.intBits; }
    static SimValue write(i64 v) { return SimValue::knownI64(v); }
};
template <> struct Number<float> {
    static constexpr ValType type = ValType::F32;
    static float read(const SimValue& v) { return v.asF32(); }
    static SimValue write(float v) { return SimValue::knownF32(v); }
};
template <> struct Number<double> {
    static constexpr ValType type = ValType::F64;
    static double read(const SimValue& v) { return v.asF64(); }
    static SimValue write(double v) { return SimValue::knownF64(v); }
};

using ValueResult = Result<SimValue, Bail>;

template <typename T, typename R = T>
CallMock unary(R (*fn)(T)) {
    return [fn](Frame& frame) -> ValueResult {
        auto arg = frame.popChecked();
        if (!arg.isOk()) return arg;
        const SimValue& v = arg.value();
        if (v.type != Number<T>::type) {
            return ValueResult::err({Bail::Reason::BadOperand, frame.pc(), "Math 参数类型不匹配"});
        }
        return ValueResult::ok(v.known ? Number<R>::write(fn(Number<T>::read(v)))
                                      : SimValue::unknownOf(Number<R>::type));
    };
}

template <typename T>
CallMock binary(T (*fn)(T, T)) {
    return [fn](Frame& frame) -> ValueResult {
        auto right = frame.popChecked();
        if (!right.isOk()) return right;
        auto left = frame.popChecked();
        if (!left.isOk()) return left;
        const SimValue& a = left.value();
        const SimValue& b = right.value();
        if (a.type != Number<T>::type || b.type != Number<T>::type) {
            return ValueResult::err({Bail::Reason::BadOperand, frame.pc(), "Math 参数类型不匹配"});
        }
        return ValueResult::ok(a.known && b.known
            ? Number<T>::write(fn(Number<T>::read(a), Number<T>::read(b)))
            : SimValue::unknownOf(Number<T>::type));
    };
}

template <typename T>
T absInt(T v) {
    return v < 0 && v != (std::numeric_limits<T>::min)() ? -v : v;
}

template <typename T, bool maximum>
T extremum(T a, T b) {
    if constexpr (std::numeric_limits<T>::is_iec559) {
        if (std::isnan(a)) return a;
        if (std::isnan(b)) return b;
        if (a == 0 && b == 0) {
            // Java 对两种零作全序比较，max 选 +0，min 选 -0。
            return std::signbit(a) == maximum ? b : a;
        }
    }
    return (maximum ? b > a : b < a) ? b : a;
}

template <typename T>
T signum(T v) {
    return v == 0 || std::isnan(v) ? v : std::copysign(T{1}, v);
}

template <typename T, typename I>
I round(T v) {
    if (std::isnan(v)) return 0;
    const T limit = std::ldexp(T{1}, std::numeric_limits<I>::digits);
    if (v >= limit) return (std::numeric_limits<I>::max)();
    if (v <= -limit) return (std::numeric_limits<I>::min)();

    // 拆整数与小数，避免 v + 0.5 在半数邻域或大整数上先发生浮点舍入。
    T whole;
    const T fraction = std::modf(v, &whole);
    const I truncated = static_cast<I>(whole);
    if (fraction >= T{0.5}) return truncated + 1;
    if (fraction < T{-0.5}) return truncated - 1;
    return truncated;
}

template <typename T>
void installExtrema(Session& session, char descriptor) {
    const std::string signature = std::string("(") + descriptor + descriptor + ")" + descriptor;
    session.callMocks["java/lang/Math.min" + signature] = binary<T>(extremum<T, false>);
    session.callMocks["java/lang/Math.max" + signature] = binary<T>(extremum<T, true>);
}

} // namespace detail

/// 注册 abs/min/max、signum、round、floor/ceil 和 sqrt。按参数类型传播 Unknown。
inline void install(Session& session) {
    using namespace detail;
    auto& mocks = session.callMocks;
    mocks["java/lang/Math.abs(I)I"] = unary<i32>(absInt<i32>);
    mocks["java/lang/Math.abs(J)J"] = unary<i64>(absInt<i64>);
    mocks["java/lang/Math.abs(F)F"] = unary<float>(std::fabs);
    mocks["java/lang/Math.abs(D)D"] = unary<double>(std::fabs);
    installExtrema<i32>(session, 'I');
    installExtrema<i64>(session, 'J');
    installExtrema<float>(session, 'F');
    installExtrema<double>(session, 'D');
    mocks["java/lang/Math.signum(F)F"] = unary<float>(signum<float>);
    mocks["java/lang/Math.signum(D)D"] = unary<double>(signum<double>);
    mocks["java/lang/Math.round(F)I"] = unary<float, i32>(round<float, i32>);
    mocks["java/lang/Math.round(D)J"] = unary<double, i64>(round<double, i64>);
    mocks["java/lang/Math.floor(D)D"] = unary<double>(std::floor);
    mocks["java/lang/Math.ceil(D)D"] = unary<double>(std::ceil);
    mocks["java/lang/Math.sqrt(D)D"] = unary<double>(std::sqrt);
}

} // namespace jsim::math
