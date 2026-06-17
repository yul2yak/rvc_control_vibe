#pragma once

#include "application/cleaning_service.hpp"
#include "ports/cleaning_session.hpp"

namespace rvc::adapters
{

class CleaningSessionAdapter : public ports::ICleaningSession
{
public:
    explicit CleaningSessionAdapter(application::CleaningService& service);

    void Start() override;
    void Tick() override;
    bool IsRunning() const override;

private:
    application::CleaningService& service_;
};

}  // namespace rvc::adapters
