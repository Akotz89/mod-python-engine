#ifndef MOD_PYTHON_ENGINE_MUTABLE_H
#define MOD_PYTHON_ENGINE_MUTABLE_H

#include "Define.h"
#include "SharedDefines.h"
#include "PythonBorrowed.h"
#include <string>
#include <cmath>
#include <type_traits>

namespace PyEng::Bridge
{
    /**
     * @brief Wrapper for mutable primitive references
     *
     * Wraps references to primitives (int, float, etc.) so they can be passed
     * through Boost.Python and modified from Python scripts.
     */
    template<typename T>
    class Mutable
    {
    public:
        explicit Mutable(T& ref) : state(std::make_shared<State>(State{ref, currentCallbackLease, {}}))
        {
            CheckLease(state->lease);
            state->lease->commits.emplace_back([value = state, &ref] { ref = std::move(value->value); });
        }

        template<typename Enum>
        static Mutable ForEnum(Enum& ref)
        {
            Mutable value(static_cast<T>(ref), 0);
            if constexpr (std::is_same_v<Enum, Powers>)
                value.state->validate = [](T replacement)
                {
                    if (static_cast<uint64>(replacement) >= MAX_POWERS)
                        RejectValue("power must name a native power slot");
                };
            else if constexpr (std::is_same_v<Enum, ServerSideVisibilityType>)
                value.state->validate = [](T replacement)
                {
                    if (static_cast<uint64>(replacement) >= TOTAL_SERVERSIDE_VISIBILITY_TYPES)
                        RejectValue("visibility type must name a native visibility slot");
                };
            value.state->lease->commits.emplace_back([state = value.state, &ref]
            {
                ref = static_cast<Enum>(state->value);
            });
            return value;
        }

        T get() const { CheckLease(state->lease); return state->value; }
        void set(T value)
        {
            CheckLease(state->lease);
            if constexpr (std::is_floating_point_v<T>)
                if (!std::isfinite(value))
                    RejectValue("native floating values must be finite");
            if (state->validate)
                state->validate(value);
            state->value = value;
        }
        operator T() const { return get(); }
        std::string repr() const
        {
            if constexpr (std::is_same_v<T, std::string>)
                return "<Mutable: " + get() + ">";
            else
                return "<Mutable: " + std::to_string(get()) + ">";
        }

    private:
        static void RejectValue(char const* message)
        {
            PyErr_SetString(PyExc_ValueError, message);
            boost::python::throw_error_already_set();
        }

        Mutable(T value, int) : state(std::make_shared<State>(State{value, currentCallbackLease, {}}))
        {
            CheckLease(state->lease);
        }

        struct State
        {
            T value;
            std::shared_ptr<CallbackLease> lease;
            std::function<void(T)> validate;
        };
        std::shared_ptr<State> state;
    };

    // Type aliases for common primitives
    using Int8Ref = Mutable<int8>;
    using Int16Ref = Mutable<int16>;
    using Int32Ref = Mutable<int32>;
    using Int64Ref = Mutable<int64>;
    using UInt8Ref = Mutable<uint8>;
    using UInt16Ref = Mutable<uint16>;
    using UInt32Ref = Mutable<uint32>;
    using UInt64Ref = Mutable<uint64>;
    using FloatRef = Mutable<float>;
    using DoubleRef = Mutable<double>;
    using BoolRef = Mutable<bool>;

} // namespace PyEng::Bridge

#endif // MOD_PYTHON_ENGINE_MUTABLE_H
