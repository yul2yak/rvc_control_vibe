#include "scenario_definitions.hpp"
#include "scenario_runner.hpp"
#include "simulator_service.hpp"

namespace
{

std::string TurnStrategyToString(rvc::simulator::ScenarioTurnStrategy strategy)
{
    return strategy == rvc::simulator::ScenarioTurnStrategy::ForceRight ? "force_right" : "prefer_left";
}

void RunThroughSimulatorService()
{
    for (const rvc::simulator::ScenarioDefinition& definition : rvc::simulator::GetScenarioDefinitions())
    {
        rvc::simulator::SimulatorService service;
        service.Reset();
        service.Start(TurnStrategyToString(definition.turn_strategy));

        for (const rvc::simulator::ScenarioStepDefinition& step : definition.steps)
        {
            service.Tick(step.sensors, step.timer_advance);
        }
    }
}

}  // namespace

int main()
{
    const rvc::simulator::ScenarioRunner runner;
    const rvc::simulator::ScenarioRunSummary summary = runner.RunAll();

    RunThroughSimulatorService();

    return summary.failed > 0 ? 1 : 0;
}
