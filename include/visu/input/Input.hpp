#pragma once


//##TODO complete input whit ALL i want (maybe think about optimisation later)
namespace ee::input
{
    struct InputState
    {
        float moveX = 0.0f;
        float moveZ = 0.0f;
    };

    InputState &state();
}
