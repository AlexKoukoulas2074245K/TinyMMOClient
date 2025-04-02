///------------------------------------------------------------------------------------------------
///  SimulationRunner.h
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 02/04/2025
///------------------------------------------------------------------------------------------------

#ifndef SimulationRunner_h
#define SimulationRunner_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/StringUtils.h>

///------------------------------------------------------------------------------------------------

namespace simulation
{
    void RunGenericSimulation(const std::string& simulationName, const int simulationIterations, std::function<void(long long)> iterationLambda, std::function<void()> resultsLambda);
    void RunStatsSimulation(const int simulationIterations);
}

///------------------------------------------------------------------------------------------------

#endif /* SimulationRunner_h */
