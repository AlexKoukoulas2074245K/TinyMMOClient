///------------------------------------------------------------------------------------------------
///  main.cpp
///  Predators
///
///  Created by Alex Koukoulas on 19/09/2023
///------------------------------------------------------------------------------------------------

#include <gtest/gtest.h>

///------------------------------------------------------------------------------------------------

extern int BATTLE_SIMULATION_ITERATIONS;

///------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    if (argc > 1) BATTLE_SIMULATION_ITERATIONS = std::stoi(argv[1]);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

///------------------------------------------------------------------------------------------------