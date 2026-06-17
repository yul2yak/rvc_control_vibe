#pragma once

namespace rvc::domain
{

struct SensorSnapshot
{
    bool front_blocked = false;
    bool left_blocked = false;
    bool right_blocked = false;
    bool dust_detected = false;
};

}  // namespace rvc::domain
