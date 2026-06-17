#include "adapters/in_memory/in_memory_sensor_reader.hpp"

namespace rvc::adapters
{

void InMemorySensorReader::SetSnapshots(const std::vector<domain::SensorSnapshot>& snapshots)
{
    snapshots_ = snapshots;
    index_ = 0;
}

void InMemorySensorReader::Reset()
{
    index_ = 0;
}

domain::SensorSnapshot InMemorySensorReader::Read() const
{
    if (snapshots_.empty())
    {
        return {};
    }

    const std::size_t current_index = index_;
    if (index_ + 1 < snapshots_.size())
    {
        ++index_;
    }

    return snapshots_.at(current_index);
}

}  // namespace rvc::adapters
