#pragma once

#include <chrono>

namespace rvc::ports
{

class ITimer
{
public:
    virtual ~ITimer() = default;

    virtual void StartBoost(std::chrono::milliseconds duration) = 0;
    virtual bool IsBoostExpired() const = 0;
    virtual void CancelBoost() = 0;
};

}  // namespace rvc::ports
