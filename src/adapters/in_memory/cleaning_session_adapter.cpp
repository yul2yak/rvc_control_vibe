#include "adapters/in_memory/cleaning_session_adapter.hpp"

namespace rvc::adapters
{

CleaningSessionAdapter::CleaningSessionAdapter(application::CleaningService& service)
    : service_(service)
{
}

void CleaningSessionAdapter::Start()
{
    service_.Start();
}

void CleaningSessionAdapter::Tick()
{
    service_.Tick();
}

bool CleaningSessionAdapter::IsRunning() const
{
    return service_.IsRunning();
}

}  // namespace rvc::adapters
