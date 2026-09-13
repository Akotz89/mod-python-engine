#ifndef MOD_PYTHON_ENGINE_SAFE_EVENT_H
#define MOD_PYTHON_ENGINE_SAFE_EVENT_H

#include "Define.h"
#include <string>
#include <variant>
#include <vector>

namespace PyEng::Events
{
using Value = std::variant<bool, int64, uint64, double, std::string>;

struct Field
{
    std::string name;
    Value value;
};

using Fields = std::vector<Field>;

inline constexpr char PartyVendorOpened[] = "party.vendor_open";
inline constexpr char PartyMailboxOpened[] = "party.mailbox_open";
} // namespace PyEng::Events

#endif // MOD_PYTHON_ENGINE_SAFE_EVENT_H
