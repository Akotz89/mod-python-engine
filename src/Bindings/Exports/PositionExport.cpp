#include "PythonHeaders.h"
#include "PythonBorrowed.h"
#include "Position.h"

namespace bp = boost::python;
using PyEng::Bridge::BorrowedPolicy;
using PyEng::Bridge::RegisterBorrowedType;

/**
 * @brief Position struct - 3D coordinates with orientation
 */
void export_position_struct()
{
    bp::class_<Position> position_struct("Position", bp::no_init);

    // coordinates and orientation
    position_struct.def("GetPositionX", &Position::GetPositionX, BorrowedPolicy<>());
    position_struct.def("GetPositionY", &Position::GetPositionY, BorrowedPolicy<>());
    position_struct.def("GetPositionZ", &Position::GetPositionZ, BorrowedPolicy<>());
    position_struct.def("GetOrientation", &Position::GetOrientation, BorrowedPolicy<>());

    // distance helpers
    position_struct.def("GetExactDist2d",
                        static_cast<float (Position::*)(float, float) const>(&Position::GetExactDist2d), BorrowedPolicy<>());
    position_struct.def("GetExactDist",
                        static_cast<float (Position::*)(float, float, float) const>(&Position::GetExactDist), BorrowedPolicy<>());
    position_struct.def("GetExactDist2dSq",
                        static_cast<float (Position::*)(float, float) const>(&Position::GetExactDist2dSq), BorrowedPolicy<>());
    position_struct.def("GetExactDistSq",
                        static_cast<float (Position::*)(float, float, float) const>(&Position::GetExactDistSq), BorrowedPolicy<>());
    position_struct.def("GetExactDist2dTo",
                        static_cast<float (Position::*)(Position const&) const>(&Position::GetExactDist2d), BorrowedPolicy<>());
    position_struct.def("GetExactDistTo",
                        static_cast<float (Position::*)(Position const&) const>(&Position::GetExactDist), BorrowedPolicy<>());
    position_struct.def("GetExactDist2dSqTo",
                        static_cast<float (Position::*)(Position const&) const>(&Position::GetExactDist2dSq), BorrowedPolicy<>());
    position_struct.def("GetExactDistSqTo",
                        static_cast<float (Position::*)(Position const&) const>(&Position::GetExactDistSq), BorrowedPolicy<>());

    // angle helpers
    position_struct.def("GetAngle", static_cast<float (Position::*)(float, float) const>(&Position::GetAngle), BorrowedPolicy<>());

    // validity
    position_struct.def("IsPositionValid", &Position::IsPositionValid, BorrowedPolicy<>());
}

/**
 * @brief WorldLocation class - Position with map context
 * @note Inherits from Position
 */
void export_worldlocation_class()
{
    bp::class_<WorldLocation, bp::bases<Position>> worldlocation_class("WorldLocation", bp::no_init);

    // map id accessors
    worldlocation_class.def("GetMapId", &WorldLocation::GetMapId, BorrowedPolicy<>());
    worldlocation_class.def("SetMapId", &WorldLocation::SetMapId, BorrowedPolicy<>());
}
