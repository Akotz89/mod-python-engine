#ifndef MOD_PYTHON_ENGINE_OBJECT_ADAPTER_H
#define MOD_PYTHON_ENGINE_OBJECT_ADAPTER_H

#include "Object.h"
#include <stdexcept>

namespace PyEng::ObjectAdapter
{
    inline void CheckIndex(Object const* object, uint16 index)
    {
        if (!object || index >= object->GetValuesCount())
            throw std::out_of_range("object field index is outside its value array");
    }
    inline uint32 GetUInt32Value(Object const* object, uint16 index)
    {
        CheckIndex(object, index);
        return object->GetUInt32Value(index);
    }
    inline float GetFloatValue(Object const* object, uint16 index)
    {
        CheckIndex(object, index);
        return object->GetFloatValue(index);
    }
    inline void SetUInt32Value(Object* object, uint16 index, uint32 value)
    {
        CheckIndex(object, index);
        object->SetUInt32Value(index, value);
    }
    inline void SetFloatValue(Object* object, uint16 index, float value)
    {
        CheckIndex(object, index);
        object->SetFloatValue(index, value);
    }
    inline bool HasFlag(Object const* object, uint16 index, uint32 flag)
    {
        CheckIndex(object, index);
        return object->HasFlag(index, flag);
    }
    inline void SetFlag(Object* object, uint16 index, uint32 flag)
    {
        CheckIndex(object, index);
        object->SetFlag(index, flag);
    }
    inline void RemoveFlag(Object* object, uint16 index, uint32 flag)
    {
        CheckIndex(object, index);
        object->RemoveFlag(index, flag);
    }
}

#endif
