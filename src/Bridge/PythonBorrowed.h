#ifndef MOD_PYTHON_ENGINE_BORROWED_H
#define MOD_PYTHON_ENGINE_BORROWED_H

#include "PythonHeaders.h"
#include <functional>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

namespace PyEng::Bridge
{
    struct CallbackLease
    {
        bool active = true;
        std::thread::id thread = std::this_thread::get_id();
        std::vector<std::function<void()>> commits;
    };

    inline thread_local std::shared_ptr<CallbackLease> currentCallbackLease;

    inline void CheckLease(std::shared_ptr<CallbackLease> const& lease)
    {
        if (!lease || !lease->active)
        {
            PyErr_SetString(PyExc_ReferenceError, "native reference expired when its callback returned");
            boost::python::throw_error_already_set();
        }
        if (lease->thread != std::this_thread::get_id())
        {
            PyErr_SetString(PyExc_RuntimeError, "native references must be used on their callback thread");
            boost::python::throw_error_already_set();
        }
    }

    struct BorrowedReference
    {
        PyObject* weakref;
        std::shared_ptr<CallbackLease> lease;
        void const* pointer;
        bool valid = true;
    };

    inline std::unordered_map<PyObject*, BorrowedReference> borrowedReferences;
    struct BorrowedType
    {
        PyTypeObject* type;
        std::function<void const*(PyObject*)> pointer;
    };
    inline std::vector<BorrowedType> borrowedTypes;

    inline bool WeakReferenceMatches(PyObject* weakref, PyObject* target)
    {
#if PY_VERSION_HEX >= 0x030D0000
        PyObject* value = nullptr;
        int status = PyWeakref_GetRef(weakref, &value);
        if (status < 0)
            boost::python::throw_error_already_set();
        bool matches = status == 0 ? target == Py_None : value == target;
        Py_XDECREF(value);
        return matches;
#else
        return PyWeakref_GetObject(weakref) == target;
#endif
    }

    inline void PruneBorrowedReferences()
    {
        for (auto it = borrowedReferences.begin(); it != borrowedReferences.end();)
        {
            if (WeakReferenceMatches(it->second.weakref, Py_None))
            {
                Py_DECREF(it->second.weakref);
                it = borrowedReferences.erase(it);
            }
            else
                ++it;
        }
    }

    // Must run with the GIL before Py_FinalizeEx; no static Python refs may survive it.
    inline void ClearBorrowedReferences()
    {
        for (auto const& entry : borrowedReferences)
            Py_DECREF(entry.second.weakref);
        borrowedReferences.clear();
        borrowedTypes.clear();
    }

    class CallbackScope
    {
    public:
        CallbackScope() : previous(currentCallbackLease), lease(std::make_shared<CallbackLease>())
        {
            PruneBorrowedReferences();
            currentCallbackLease = lease;
        }
        ~CallbackScope()
        {
            for (auto const& commit : lease->commits)
                commit();
            lease->commits.clear();
            lease->active = false;
            currentCallbackLease = previous;
        }
        CallbackScope(CallbackScope const&) = delete;
        CallbackScope& operator=(CallbackScope const&) = delete;

    private:
        std::shared_ptr<CallbackLease> previous;
        std::shared_ptr<CallbackLease> lease;
    };

    template<typename T>
    inline void RegisterBorrowedType(boost::python::object const& type)
    {
        borrowedTypes.push_back({reinterpret_cast<PyTypeObject*>(type.ptr()), [](PyObject* value) -> void const*
        {
            boost::python::object object(boost::python::handle<>(boost::python::borrowed(value)));
            return boost::python::extract<T const*>(object);
        }});
    }

    inline BorrowedType const* FindBorrowedType(PyObject* value)
    {
        for (BorrowedType const& type : borrowedTypes)
            if (PyObject_TypeCheck(value, type.type))
                return &type;
        return nullptr;
    }

    inline bool IsBorrowedType(PyObject* value) { return FindBorrowedType(value) != nullptr; }

    inline void InvalidateBorrowedPointer(void const* pointer)
    {
        for (auto& entry : borrowedReferences)
            if (entry.second.pointer == pointer)
                entry.second.valid = false;
    }

    inline void TrackBorrowedReference(PyObject* value)
    {
        if (!value || !IsBorrowedType(value))
            return;
        CheckLease(currentCallbackLease);
        auto found = borrowedReferences.find(value);
        if (found != borrowedReferences.end())
        {
            // A wrapper already borrowed by the outer callback remains valid there.
            if (WeakReferenceMatches(found->second.weakref, value) && found->second.lease->active && found->second.valid)
                return;
            Py_DECREF(found->second.weakref);
            borrowedReferences.erase(found);
        }
        void const* pointer = FindBorrowedType(value)->pointer(value);
        PyObject* weakref = PyWeakref_NewRef(value, nullptr);
        if (!weakref)
            boost::python::throw_error_already_set();
        try
        {
            borrowedReferences.emplace(value, BorrowedReference{weakref, currentCallbackLease, pointer});
        }
        catch (...)
        {
            Py_DECREF(weakref);
            throw;
        }
    }

    inline void CheckBorrowedReference(PyObject* value)
    {
        if (!IsBorrowedType(value))
            return;
        auto found = borrowedReferences.find(value);
        if (found == borrowedReferences.end() || !WeakReferenceMatches(found->second.weakref, value) || !found->second.valid)
        {
            PyErr_SetString(PyExc_ReferenceError, "native reference has no active callback owner");
            boost::python::throw_error_already_set();
        }
        CheckLease(found->second.lease);
    }

    template<typename Base = boost::python::default_call_policies>
    struct BorrowedPolicy : Base
    {
        static bool precall(PyObject* args)
        {
            for (Py_ssize_t index = 0; index < PyTuple_GET_SIZE(args); ++index)
                CheckBorrowedReference(PyTuple_GET_ITEM(args, index));
            return Base::precall(args);
        }

        static PyObject* postcall(PyObject* args, PyObject* result)
        {
            result = Base::postcall(args, result);
            if (result)
            {
                try
                {
                    TrackBorrowedReference(result);
                }
                catch (...)
                {
                    Py_DECREF(result);
                    throw;
                }
            }
            return result;
        }
    };
}

#endif
