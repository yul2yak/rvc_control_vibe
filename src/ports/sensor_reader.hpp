#pragma once

#include "domain/sensor_snapshot.hpp"

namespace rvc::ports
{

class ISensorReader
{
public:
    virtual ~ISensorReader() = default;

    virtual domain::SensorSnapshot Read() const = 0;
};

}  // namespace rvc::ports
