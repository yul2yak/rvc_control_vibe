#include "adapters/in_memory/fake_timer.hpp"

namespace rvc::adapters
{

void FakeTimer::StartBoost(std::chrono::milliseconds duration)
{
    active_ = true;
    elapsed_ = std::chrono::milliseconds{0};
    duration_ = duration;
}

bool FakeTimer::IsBoostExpired() const
{
    if (!active_)
    {
        return false;
    }

    return elapsed_ >= duration_;
}

void FakeTimer::CancelBoost()
{
    active_ = false;
    elapsed_ = std::chrono::milliseconds{0};
    duration_ = std::chrono::milliseconds{0};
}

void FakeTimer::Advance(std::chrono::milliseconds delta)
{
    if (active_)
    {
        elapsed_ += delta;
    }
}

}  // namespace rvc::adapters
