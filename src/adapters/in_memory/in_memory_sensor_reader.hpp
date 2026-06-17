#pragma once

#include <vector>

#include "domain/sensor_snapshot.hpp"
#include "ports/sensor_reader.hpp"

namespace rvc::adapters
{

class InMemorySensorReader : public ports::ISensorReader
{
public:
    void SetSnapshots(const std::vector<domain::SensorSnapshot>& snapshots);
    void Reset();

    domain::SensorSnapshot Read() const override;

private:
    std::vector<domain::SensorSnapshot> snapshots_;
    mutable std::size_t index_ = 0;
};

}  // namespace rvc::adapters
