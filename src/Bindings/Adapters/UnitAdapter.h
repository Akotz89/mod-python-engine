#ifndef MOD_PYTHON_ENGINE_UNIT_ADAPTER_H
#define MOD_PYTHON_ENGINE_UNIT_ADAPTER_H

#include "Unit.h"
#include <stdexcept>

namespace PyEng::UnitAdapter
{
    inline void CheckPower(Powers power)
    {
        if (static_cast<uint32>(power) >= MAX_POWERS)
            throw std::out_of_range("power must name a native power slot");
    }

    inline uint32 GetPower(Unit const* unit, Powers power)
    {
        CheckPower(power);
        return unit->GetPower(power);
    }

    inline uint32 GetMaxPower(Unit const* unit, Powers power)
    {
        CheckPower(power);
        return unit->GetMaxPower(power);
    }

    inline uint32 GetGUIDLow(Unit* unit)
    {
        return unit ? unit->GetGUID().GetCounter() : 0;
    }

} // namespace PyEng::UnitAdapter

#endif // MOD_PYTHON_ENGINE_UNIT_ADAPTER_H
