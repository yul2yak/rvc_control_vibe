#pragma once

namespace rvc::ports
{

class ICleaningSession
{
public:
    virtual ~ICleaningSession() = default;

    virtual void Start() = 0;
    virtual void Tick() = 0;
    virtual bool IsRunning() const = 0;
};

}  // namespace rvc::ports
