#include "Part.h"

namespace Horizon {

Part::Part()
    : Position{0.0f, 0.0f, 0.0f}
    , Size{1.0f, 1.0f, 1.0f}
    , Color{255, 255, 255}
{
}

std::string Part::GetClass()
{
    return "Part";
}

} // namespace Horizon
