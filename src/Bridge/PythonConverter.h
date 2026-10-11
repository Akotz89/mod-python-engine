#ifndef MOD_PYTHON_ENGINE_CONVERTER_H
#define MOD_PYTHON_ENGINE_CONVERTER_H

#include "PythonAPI.h"
#include "PythonMutable.h"
#include "PythonNativeReference.h"
#include <chrono>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

class Creature;
class Unit;
class WorldObject;
class Map;

namespace PyEng::Bridge
{
    template<typename T, typename = void> struct IsRange : std::false_type {};
    template<typename T> struct IsRange<T, std::void_t<decltype(std::begin(std::declval<T const&>())),
        decltype(std::end(std::declval<T const&>()))>> : std::true_type {};
    template<typename T, typename = void> struct IsMap : std::false_type {};
    template<typename T> struct IsMap<T, std::void_t<typename T::mapped_type>> : std::true_type {};
    template<typename T, typename = void> struct IsComplete : std::false_type {};
    template<typename T> struct IsComplete<T, std::void_t<decltype(sizeof(T))>> : std::true_type {};

    class Converter
    {
    public:
        template<typename T>
        static API::Object ToPython(std::reference_wrapper<T> ref)
        {
            T& value = ref.get();
            if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string>)
                return API::Object(Mutable<T>(value));
            else if constexpr (std::is_enum_v<T>)
                return API::Object(Mutable<std::underlying_type_t<T>>::ForEnum(value));
            else if constexpr (IsRange<T>::value || std::is_pointer_v<T>)
            {
                auto copied = std::make_shared<T>(value);
                CheckLease(currentCallbackLease);
                currentCallbackLease->commits.emplace_back([copied, &value] { value = std::move(*copied); });
                return MakeMutableValue([copied] { return ToPython(*copied); },
                    [copied](API::Object const& replacement) { *copied = FromPython<T>(replacement); });
            }
            else
                return ToPython(&value);
        }

        template<typename T>
        static API::Object ToPython(T* ptr)
        {
            if (!ptr)
                return API::Object();
            if constexpr (IsComplete<T>::value && !std::is_void_v<T>)
            {
                auto registration = boost::python::converter::registry::query(
                    boost::python::type_id<std::remove_cv_t<T>>());
                if (registration && registration->m_class_object)
                {
                    API::Object result(API::Ptr(ptr));
                    TrackBorrowedReference(result.ptr());
                    return result;
                }
            }
            if constexpr (IsRange<T>::value && !std::is_same_v<std::remove_cv_t<T>, std::string>)
                return ToPython(*ptr);
            if constexpr (!std::is_same_v<std::remove_cv_t<T>, Creature> && std::is_convertible_v<T*, Creature const*>)
            {
                using Base = std::conditional_t<std::is_const_v<T>, Creature const, Creature>;
                return ToPython(static_cast<Base*>(ptr));
            }
            else if constexpr (!std::is_same_v<std::remove_cv_t<T>, Unit> && std::is_convertible_v<T*, Unit const*>)
            {
                using Base = std::conditional_t<std::is_const_v<T>, Unit const, Unit>;
                return ToPython(static_cast<Base*>(ptr));
            }
            else if constexpr (!std::is_same_v<std::remove_cv_t<T>, WorldObject> && std::is_convertible_v<T*, WorldObject const*>)
            {
                using Base = std::conditional_t<std::is_const_v<T>, WorldObject const, WorldObject>;
                return ToPython(static_cast<Base*>(ptr));
            }
            else if constexpr (!std::is_same_v<std::remove_cv_t<T>, Map> && std::is_convertible_v<T*, Map const*>)
            {
                using Base = std::conditional_t<std::is_const_v<T>, Map const, Map>;
                return ToPython(static_cast<Base*>(ptr));
            }
            return API::Object(NativeReference(ptr));
        }

        template<typename T>
        static API::Object ToPython(std::shared_ptr<T> const& value) { return ToPython(value.get()); }

        template<typename A, typename B>
        static API::Object ToPython(std::pair<A, B> const& value)
        {
            return boost::python::make_tuple(ToPython(value.first), ToPython(value.second));
        }

        template<typename Clock, typename Duration>
        static API::Object ToPython(std::chrono::time_point<Clock, Duration> const& value)
        {
            return API::Object(std::chrono::duration_cast<std::chrono::milliseconds>(value.time_since_epoch()).count());
        }

        template<typename T>
        static API::Object ToPython(T const& value)
        {
            if constexpr (IsMap<T>::value)
            {
                API::Dict result;
                for (auto const& entry : value)
                    result[ToPython(entry.first)] = ToPython(entry.second);
                return result;
            }
            else if constexpr (IsRange<T>::value && !std::is_same_v<T, std::string>)
            {
                API::List result;
                for (auto const& entry : value)
                    result.append(ToPython(entry));
                return result;
            }
            else if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string>)
                return API::Object(value);
            else if constexpr (std::is_enum_v<T>)
            {
                auto registration = boost::python::converter::registry::query(boost::python::type_id<T>());
                if (registration && registration->m_to_python)
                    return API::Object(value);
                return API::Object(static_cast<std::underlying_type_t<T>>(value));
            }
            else
            {
                auto registration = boost::python::converter::registry::query(boost::python::type_id<T>());
                if (registration && registration->m_to_python)
                {
                    API::Object result(value);
                    TrackBorrowedReference(result.ptr());
                    return result;
                }
                return ToPython(&value);
            }
        }

        static API::Object ToPython(std::nullptr_t) { return API::Object(); }

        static API::Object ToPython(char const* str)
        {
            return str ? API::FromString(std::string(str)) : API::Object();
        }
        static API::Object ToPython(char* str) { return ToPython(static_cast<char const*>(str)); }

        template<typename T>
        static T FromPython(API::Object const& value)
        {
            if constexpr (std::is_pointer_v<T>)
            {
                if (API::IsNone(value))
                    return nullptr;
                boost::python::extract<NativeReference const&> opaque(value);
                if (opaque.check())
                    return opaque().template Get<std::remove_pointer_t<T>>();
                CheckBorrowedReference(value.ptr());
                return boost::python::extract<T>(value);
            }
            else if constexpr (IsMap<T>::value)
            {
                T result;
                API::Dict dict = boost::python::extract<API::Dict>(value);
                API::List keys = dict.keys();
                for (Py_ssize_t index = 0; index < boost::python::len(keys); ++index)
                    result.emplace(FromPython<typename T::key_type>(keys[index]),
                        FromPython<typename T::mapped_type>(dict[keys[index]]));
                return result;
            }
            else if constexpr (IsRange<T>::value && !std::is_same_v<T, std::string>)
            {
                T result;
                for (Py_ssize_t index = 0; index < boost::python::len(value); ++index)
                    result.insert(result.end(), FromPython<typename T::value_type>(value[index]));
                return result;
            }
            else
            {
                CheckBorrowedReference(value.ptr());
                return boost::python::extract<T>(value);
            }
        }

    private:
        static API::Object MakeMutableValue(std::function<API::Object()> getter,
                                             std::function<void(API::Object const&)> setter);
    };

    class MutableValue
    {
    public:
        MutableValue(std::function<API::Object()> getter, std::function<void(API::Object const&)> setter)
            : getter(std::move(getter)), setter(std::move(setter)), lease(currentCallbackLease) {}
        API::Object get() const { CheckLease(lease); return getter(); }
        void set(API::Object const& value) { CheckLease(lease); setter(value); }
    private:
        std::function<API::Object()> getter;
        std::function<void(API::Object const&)> setter;
        std::shared_ptr<CallbackLease> lease;
    };

    inline API::Object Converter::MakeMutableValue(std::function<API::Object()> getter,
                                                   std::function<void(API::Object const&)> setter)
    {
        return API::Object(MutableValue(std::move(getter), std::move(setter)));
    }
}

#endif
