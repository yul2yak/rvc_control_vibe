#pragma once

#include <chrono>

#include "ports/timer.hpp"

namespace rvc::adapters
{

class FakeTimer : public ports::ITimer
{
public:
    void StartBoost(std::chrono::milliseconds duration) override;
    bool IsBoostExpired() const override;
    void CancelBoost() override;
    void Advance(std::chrono::milliseconds delta);

private:
    bool active_ = false;
    std::chrono::milliseconds elapsed_{0};
    std::chrono::milliseconds duration_{0};
};

}  // namespace rvc::adapters
