///------------------------------------------------------------------------------------------------
///  main.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 19/09/2023
///------------------------------------------------------------------------------------------------

///------------------------------------------------------------------------------------------------

#if defined(RUN_TESTS) && !defined(NDEBUG)
#include <gtest/gtest.h>
#include <engine/utils/PlatformMacros.h>

#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleUtils.h>
#endif

///------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
#if defined(MACOS) || defined(MOBILE_FLOW)
    apple_utils::SetAssetFolder();
#endif
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

///------------------------------------------------------------------------------------------------

#else
#if defined(USE_EDITOR)
#include <editor/Editor.h>
#include <stdlib.h>
#include <SDL.h>

///------------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    Editor editor(argc, argv);
    return 0;
}

///------------------------------------------------------------------------------------------------
#else
#include <game/Game.h>
#include <stdlib.h>
#include <SDL.h>

///------------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    Game game(argc, argv);
    return 0;
}

///------------------------------------------------------------------------------------------------
#endif
#endif
