#include "visu/input/Input.hpp"

namespace ee::input
{
    InputState &state()
    {
        static InputState s;
        return s;
    }
}
