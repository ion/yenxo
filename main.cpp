#include <when.hpp>

// boost
#include <meta.hpp>
#include <boost/hana.hpp>

// std
#include <iostream>
#include <type_traits>
#include <string>
#include <variant.hpp>


namespace hana = boost::hana;


namespace detail {


///
/// Unified converstion of T to Variant
///
template <typename T, typename = void>
struct ToVariant : ToVariant<T, When<true>> {};


///
/// Specialization to types with `static Variant T::toVariant(T)`
///
template <typename T, bool condition>
struct ToVariant<T, When<condition>> {
    static_assert(
            rp::callable(T::toVariant, rp::Type<T>()),
            "The type T must have T::toVariant(T) static member function");

    static_assert(
            std::is_same_v<decltype(T::toVariant(std::declval<T>())), Variant>,
            "The return type of T::toVariant must be Variant");

    Variant operator()(T const& x) const { return T::toVariant(x); }
};


///
/// Specialization for types for which Variant have constructor
///
template <typename T>
struct ToVariant<T, When<rp::detail::Valid<
        decltype(Variant(std::declval<T>()))>::value>> {
    Variant operator()(T x) const { return Variant(x); }
};


///
/// Convinient shortcut function
///
template <typename T>
constexpr Variant toVariant(T&& t) {
    return ToVariant<std::decay_t<T>>()(std::forward<T>(t));
}


///
/// Unified converstion of Variant to T
///
template <typename T, typename = void>
struct FromVariant : FromVariant<T, When<true>> {};


///
/// Specialization to types with `static T T::fromVariant(Variant)`
///
template <typename T, bool condition>
struct FromVariant<T, When<condition>> {
    static_assert(
            rp::callable(T::fromVariant, rp::Type<Variant>()),
            "The type T must have static member function fromVariant(Variant)");
    static_assert(
            std::is_same_v<decltype(T::fromVariant(Variant())), T>,
            "The return type of T::fromVariant must be T");

    T operator()(Variant const& x) const { return T::fromVariant(x); }
};


///
/// Specialization for types for which Variant have conversion
///
template <typename T>
struct FromVariant<T,
        When<rp::detail::Valid<decltype(static_cast<T>(Variant()))>::value>> {
    T operator()(Variant const& x) const { return static_cast<T>(x); }
};


///
/// Convinient shortcut function
///
template <typename T>
constexpr T fromVariant(Variant const& v) {
    return FromVariant<T>()(v);
}


///
/// Convinient shortcut function
///
template <typename T>
constexpr void fromVariant(T& x, Variant const& v) {
    x = FromVariant<T>()(v);
}


} // namespace detail


namespace mixin {


///
/// Adds members
/// `static Variant toVariant(Derived)`
/// `static Derived fromVariant(Variant)`
///
template <typename Derived>
struct Var {
    static Variant toVariant(Derived const& x) {
        Variant::Map ret;

        hana::for_each(x, hana::fuse([&](auto name, auto value) {
            ret[hana::to<char const*>(name)] = detail::toVariant(value);
        }));

        return Variant(ret);
    }

    static Derived fromVariant(Variant const& x) {
        Derived ret;
        auto const& map = x.map();

        hana::for_each(hana::accessors<Derived>(),
                       hana::fuse([&](auto name, auto value) {
            auto& tmp = value(ret);
            return detail::fromVariant(
                        tmp, map.at(hana::to<char const*>(name)));
        }));

        return ret;
    }
};


} // namespace


struct X {
    static Variant toVariant(X const& x) { return {}; }
    static X fromVariant(Variant const& x) { return {}; }
};


struct Person :
        mixin::Var<Person> {
    BOOST_HANA_DEFINE_STRUCT(
            Person,
            (std::string, name),
            (int, age)
    );
};


int main() {
    Person::toVariant(Person());
}
