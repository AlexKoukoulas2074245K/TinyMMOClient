///------------------------------------------------------------------------------------------------
///  SimulationRunner.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 02/04/2025
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/utils/Logging.h>
#include <game/SimulationRunner.h>
#include <net_common/Board.h>
#include <iostream>
#include <SDL.h>

///------------------------------------------------------------------------------------------------

namespace simulation
{

///------------------------------------------------------------------------------------------------

void RunGenericSimulation(const std::string& simulationName, const int simulationIterations, std::function<void(long long)> iterationLambda, std::function<void()> resultsLambda)
{
    std::string simulationSepString = "===================== " + simulationName + " ====================";
    std::cout << simulationSepString << std::endl;
    std::cout << "Simulating " << simulationIterations << " iterations" << std::endl;
    
    auto currentMillisSinceInit = static_cast<float>(SDL_GetTicks());
    for (long long i = 0LL; i < simulationIterations; ++i)
    {
        if (i % (simulationIterations/10) == 0)
        {
            auto percentComplete = static_cast<int>((i/static_cast<float>(simulationIterations)) * 100.0f);
            std::cout << "Simulation " << percentComplete << "% complete";
            
            if (!math::FloatsSufficientlyClose(currentMillisSinceInit, static_cast<float>(SDL_GetTicks())))
            {
                auto secsSinceLastPrint = (static_cast<float>(SDL_GetTicks()) - currentMillisSinceInit)/1000.0f;
                std::cout << " (Estimated duration left: " << strutils::GetHoursMinutesSecondsStringFromSeconds(static_cast<int>((100.0f - percentComplete)/10 * secsSinceLastPrint))  << ")";
                currentMillisSinceInit = static_cast<float>(SDL_GetTicks());
            }
            
            std::cout << std::endl;
        }
        
        iterationLambda(i);
    }
    std::cout << "Simulation 100% complete" << std::endl;
    
    resultsLambda();
    std::cout << std::string(simulationSepString.size(), '=') << std::endl;
}

///------------------------------------------------------------------------------------------------

void RunStatsSimulation(const int simulationIterations)
{
    static const long long COINS_PER_SPIN = 1;
    
    slots::Board b;
    long long numberOf3Scatters = 0;
    long long numberOf4Scatters = 0;
    long long numberOf5Scatters = 0;
    long long numberOfCombos = 0;
    long long numberOf5Wilds = 0;
    long long totalReturn = 0;
    std::unordered_map<slots::WinSourceType, long long> rawWinSourceTypeContributions;

    RunGenericSimulation("WinStats", simulationIterations,
    // Iteration Lambda
    [&](long long){
        int nextSpinId = math::RandomInt();
        b.PopulateBoardForSpin(nextSpinId);
        
        while (true)
        {
            const auto& boardStateResolution = b.ResolveBoardState();
            totalReturn += COINS_PER_SPIN * boardStateResolution.mTotalWinMultiplier;
            
            if (boardStateResolution.mShouldTumble)
            {
//                if (boardStateResolution.mWinningPaylines.size() == 3 &&
//                    boardStateResolution.mWinningPaylines[0].mCombo && boardStateResolution.mWinningPaylines[0].mPayline == slots::PaylineType::PAYLINE_1 &&
//                    boardStateResolution.mWinningPaylines[1].mCombo && boardStateResolution.mWinningPaylines[1].mPayline == slots::PaylineType::PAYLINE_2 &&
//                    boardStateResolution.mWinningPaylines[2].mCombo && boardStateResolution.mWinningPaylines[2].mPayline == slots::PaylineType::PAYLINE_3)
//                {
//                    DEBUG_BREAKPOINT();
//                }
            }
            
            if (b.GetSymbolCountInPlayableBoard(slots::SymbolType::WILD) == 5)
            {
                numberOf5Wilds++;
            }
            
            if (b.GetSymbolCountInPlayableBoard(slots::SymbolType::SCATTER) == 5)
            {
                numberOf5Scatters++;
            }
            else if (b.GetSymbolCountInPlayableBoard(slots::SymbolType::SCATTER) == 4)
            {
                numberOf4Scatters++;
            }
            else if (b.GetSymbolCountInPlayableBoard(slots::SymbolType::SCATTER) == 3)
            {
                numberOf3Scatters++;
            }
            
            for (const auto& paylineData: boardStateResolution.mWinningPaylines)
            {
                rawWinSourceTypeContributions[paylineData.mWinSourceType] += paylineData.mWinMultiplier;
            }

            for (const auto& paylineData: boardStateResolution.mWinningPaylines)
            {
                if (paylineData.mCombo)
                {
                    numberOfCombos++;
                    break;
                }
            }
            
            if (boardStateResolution.mShouldTumble)
            {
                b.ResolveBoardTumble(boardStateResolution);
            }
            else
            {
                break;
            }
        }
    },
    // Footer Lambda
    [&](){
        std::cout << "Total RTP: " << ((totalReturn * COINS_PER_SPIN)/float(COINS_PER_SPIN * simulationIterations)) * 100.0f << "%" << std::endl;
        std::cout << "RTP Breakdown: " << std::endl;
        
        struct WinSourceTypeEntry
        {
            WinSourceTypeEntry(slots::WinSourceType winSourceType, float contribution)
            : mWinSourceType(winSourceType)
            , mContribution(contribution)
            {}
            
            const slots::WinSourceType mWinSourceType;
            const float mContribution;
        };

        struct WinSourceTypeEntryComparator
        {
            bool operator()(const WinSourceTypeEntry& lhs, const WinSourceTypeEntry& rhs) const
            {
                return lhs.mContribution > rhs.mContribution;
            }
        };
        
        std::set<WinSourceTypeEntry, WinSourceTypeEntryComparator> winSourceTypeContributions;
        for (const auto& contributionEntry: rawWinSourceTypeContributions)
        {
            winSourceTypeContributions.emplace(contributionEntry.first, (contributionEntry.second/static_cast<float>(totalReturn)) * 100.0f);
        }
        
        for (const auto& winSourceTypeContribution: winSourceTypeContributions)
        {
            std::cout << "    [" << slots::WIN_SOURCE_TYPE_NAMES.at(winSourceTypeContribution.mWinSourceType) << " = " << winSourceTypeContribution.mContribution << "%]" << std::endl;
        }

        std::cout << "5 Wild Chance: " << (numberOf5Wilds/float(simulationIterations)) * 100.0f << "%" << std::endl;
        std::cout << "5 Scatter Chance: " << (numberOf5Scatters/float(simulationIterations)) * 100.0f << "%" << std::endl;
        std::cout << "4 Scatter Chance: " << (numberOf4Scatters/float(simulationIterations)) * 100.0f << "%" << std::endl;
        std::cout << "3 Scatter Chance: " << (numberOf3Scatters/float(simulationIterations)) * 100.0f << "%" << std::endl;
        std::cout << "Tumble Chance: " << (numberOfCombos/float(simulationIterations)) * 100.0f << "%" << std::endl;
    });
}

///------------------------------------------------------------------------------------------------


}

///------------------------------------------------------------------------------------------------
