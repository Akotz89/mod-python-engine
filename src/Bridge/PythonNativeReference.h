#ifndef MOD_PYTHON_ENGINE_NATIVE_REFERENCE_H
#define MOD_PYTHON_ENGINE_NATIVE_REFERENCE_H

#include "PythonBorrowed.h"
#include <string>

namespace PyEng::Bridge
{
    template<typename T>
    std::string NativeTypeName()
    {
        return __PRETTY_FUNCTION__;
    }

    // Unbound upstream argument types retain their identity without dereferencing opaque memory.
    class NativeReference
    {
    public:
        template<typename T>
        explicit NativeReference(T* value)
            : pointer(value), type(NativeTypeName<T>()), lease(currentCallbackLease)
        {
            CheckLease(lease);
        }
        std::string GetTypeName() const { CheckLease(lease); return type; }
        bool IsValid() const { return lease && lease->active && lease->thread == std::this_thread::get_id(); }
        std::string repr() const { return "<NativeReference " + GetTypeName() + ">"; }
        template<typename T>
        T* Get() const
        {
            CheckLease(lease);
            if (type != NativeTypeName<T>())
            {
                PyErr_SetString(PyExc_TypeError, "native reference type does not match");
                boost::python::throw_error_already_set();
            }
            return const_cast<T*>(static_cast<T const*>(pointer));
        }

    private:
        void const* pointer;
        std::string type;
        std::shared_ptr<CallbackLease> lease;
    };
}

#endif
