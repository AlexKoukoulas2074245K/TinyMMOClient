///------------------------------------------------------------------------------------------------
///  BoardTest.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 27/02/2025
///------------------------------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <net_common/Board.h>
#include <engine/utils/MathUtils.h>

///------------------------------------------------------------------------------------------------

TEST(BoardTest, TestBasicBoardAccessors)
{
    slots::Board b;
    b.SetBoardSymbol(0, 0, slots::SymbolType::CHICKEN);
    ASSERT_EQ(b.GetBoardSymbol(0, 0), slots::SymbolType::CHICKEN);
}

///------------------------------------------------------------------------------------------------

void runSimulation(const std::string& simulationName, const int simulationSteps, std::function<void(long long)> iterationLambda, std::function<void()> resultsLambda)
{
    std::string simulationSepString = "===================== " + simulationName + " ====================";
    std::cout << simulationSepString << std::endl;
    std::cout << "Simulating " << simulationSteps << " iterations" << std::endl;
    
    for (long long i = 0LL; i < simulationSteps; ++i)
    {
        if (i % (simulationSteps/10) == 0)
        {
            auto percentComplete = static_cast<int>((i/static_cast<float>(simulationSteps)) * 100.0f);
            std::cout << "Simulation " << percentComplete << "% complete" << std::endl;
        }
        
        iterationLambda(i);
    }
    std::cout << "Simulation 100% complete" << std::endl;

    resultsLambda();
    std::cout << std::string(simulationSepString.size(), '=') << std::endl;
}

///------------------------------------------------------------------------------------------------

TEST(BoardTest, TestComplexSymbolsNotAppearingInRandomBoardPopulation)
{
    static const long long SIMULATIONS = 100000;

    slots::Board b;
    bool foundComplexSymbol = false;

    runSimulation("ComplexSymbolsNotAppearing", SIMULATIONS,
    // Iteration Lambda
    [&](long long){
        b.PopulateBoardForSpin(math::RandomInt());
        
        for (int reelIndex = 0; reelIndex < slots::BOARD_COLS; ++reelIndex)
        {
            if (b.GetSymbolCountInEntireReel(reelIndex, slots::SymbolType::STRAWBERRY_CAKE) > 0 ||
                b.GetSymbolCountInEntireReel(reelIndex, slots::SymbolType::CHOCOLATE_CAKE) > 0 ||
                b.GetSymbolCountInEntireReel(reelIndex, slots::SymbolType::ROAST_CHICKEN) > 0)
            {
                foundComplexSymbol = true;
                break;
            }
        }
    },
    // Footer Lambda
    [&](){
        ASSERT_EQ(foundComplexSymbol, false);
    });
}

///------------------------------------------------------------------------------------------------

TEST(BoardTest, TestWildAndScatterAppearOnlyOnceInEachReelInRandomBoardPopulation)
{
    static const long long SIMULATIONS = 100000;
    bool foundDuplicateWildOrScatterInReel = false;
    slots::Board b;
    
    runSimulation("Wild/ScatterAppearOnce", SIMULATIONS,
    // Iteration Lambda
    [&](long long){
        b.PopulateBoardForSpin(math::RandomInt());
        
        for (int reelIndex = 0; reelIndex < slots::BOARD_COLS; ++reelIndex)
        {
            if (b.GetSymbolCountInEntireReel(reelIndex, slots::SymbolType::WILD) > 1 ||
                b.GetSymbolCountInEntireReel(reelIndex, slots::SymbolType::SCATTER) > 1)
            {
                foundDuplicateWildOrScatterInReel = true;
                break;
            }
        }
    },
    // Footer Lambda
    [&](){
        ASSERT_EQ(foundDuplicateWildOrScatterInReel, false);
    });
}

///------------------------------------------------------------------------------------------------

TEST(BoardTest, TestRandomBoardWinStats)
{
    static const long long SIMULATIONS = 1000000;
    static const long long COINS_PER_SPIN = 1;
    
    slots::Board b;
    long long numberOf3Scatters = 0;
    long long numberOf4Scatters = 0;
    long long numberOf5Scatters = 0;
    long long numberOfTumbles = 0;
    long long numberOf5Wilds = 0;
    long long totalReturn = 0;
    std::unordered_map<slots::WinSourceType, long long> rawWinSourceTypeContributions;

    runSimulation("WinStats", SIMULATIONS,
    // Iteration Lambda
    [&](long long){
        b.PopulateBoardForSpin(math::RandomInt());
        const auto& boardStateResolution = b.ResolveBoardState();
        totalReturn += COINS_PER_SPIN * boardStateResolution.mTotalWinMultiplier;

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
            if (paylineData.mTumble)
            {
                numberOfTumbles++;
                break;
            }
        }
    },
    // Footer Lambda
    [&](){
        std::cout << "Total RTP: " << ((totalReturn * COINS_PER_SPIN)/float(COINS_PER_SPIN * SIMULATIONS)) * 100.0f << "%" << std::endl;
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

        std::cout << "5 Wild Chance: " << (numberOf5Wilds/float(SIMULATIONS)) * 100.0f << "%" << std::endl;
        std::cout << "5 Scatter Chance: " << (numberOf5Scatters/float(SIMULATIONS)) * 100.0f << "%" << std::endl;
        std::cout << "4 Scatter Chance: " << (numberOf4Scatters/float(SIMULATIONS)) * 100.0f << "%" << std::endl;
        std::cout << "3 Scatter Chance: " << (numberOf3Scatters/float(SIMULATIONS)) * 100.0f << "%" << std::endl;
        std::cout << "Tumble Chance: " << (numberOfTumbles/float(SIMULATIONS)) * 100.0f << "%" << std::endl;
    });
}

