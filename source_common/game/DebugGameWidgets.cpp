///------------------------------------------------------------------------------------------------
///  DebugGameWidgets.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 31/03/2025
///------------------------------------------------------------------------------------------------

#if defined(USE_IMGUI)
#include <engine/CoreSystemsEngine.h>
#include <engine/input/IInputStateManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/scene/SceneObjectUtils.h>
#include <engine/utils/Logging.h>
#include <game/DebugGameWidgets.h>
#include <game/BoardView.h>
#include <game/Game.h>
#include <imgui/imgui.h>
#include <SDL.h>

///------------------------------------------------------------------------------------------------

void DebugGameWidgets::CreateDebugWidgets(Game& game)
{
    ImGui::Begin("Net Stats", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    ImGui::Text("Ping %d millis", game.mLastPingMillis.load());
    ImGui::End();
    
    ImGui::Begin("Debug Data", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    ImGui::Text("Player ID: %lld", game.mPlayerId);
    ImGui::Text("Current Spin ID: %d", game.mSpinId);
    ImGui::SameLine();
    if (ImGui::Button("Copy to Clipboard"))
    {
        SDL_SetClipboardText(std::to_string(game.mSpinId).c_str());
    }
    
    ImGui::SeparatorText("Scatter Data");
    ImGui::Text("Scatter Spins Left: %d", game.mBoardModel.GetOustandingScatterSpins());
    ImGui::Text("Scatter Multiplier: %d", game.mBoardModel.GetScatterMultiplier());
    ImGui::Text("Scatter Selected Combo: %s", slots::Board::GetSymbolDebugName(game.mBoardModel.GetSelectedScatterComboSymbol()).c_str());
    
    if (game.mBoardView)
    {
        if (game.mBoardView->GetSpinAnimationState() == BoardView::SpinAnimationState::IDLE)
        {
            ImGui::SeparatorText("Magic Spins");
            static int sDebugBoardConfigTypesIndex = 0;
            static const std::string SCATTER_3 = "SCATTER_3";
            static const std::string SCATTER_5 = "SCATTER_5";
            static const std::string COMBO_1   = "COMBO_1";
            static const std::string COMBO_2   = "COMBO_2";
            static const std::string COMBO_3   = "COMBO_3";
            static const std::string COMBO_4   = "COMBO_4";

            static std::vector<std::string> sDebugBoardConfigTypes =
            {
                SCATTER_3,
                SCATTER_5,
                COMBO_1,
                COMBO_2,
                COMBO_3,
                COMBO_4
            };
            if (ImGui::BeginCombo(" ", sDebugBoardConfigTypes.at(sDebugBoardConfigTypesIndex).c_str()))
            {
                for (auto n = 0U; n < sDebugBoardConfigTypes.size(); n++)
                {
                    const bool isSelected = (sDebugBoardConfigTypesIndex == n);
                    if (ImGui::Selectable(sDebugBoardConfigTypes.at(n).c_str(), isSelected))
                    {
                        sDebugBoardConfigTypesIndex = n;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if (ImGui::Button("Magic Spin"))
            {
                slots::Board b;
                auto spinId = 0;
                while (true)
                {
                    spinId = math::RandomInt();
                    b.PopulateBoardForSpin(spinId);
                    const auto& resolution = b.ResolveBoardState();
                    
                    int numberOfCombos = 0;
                    for (const auto& payline: resolution.mWinningPaylines)
                    {
                        if (payline.mCombo)
                        {
                            numberOfCombos++;
                        }
                    }
                    
                    const auto& scatterCoordinates = b.GetSymbolCoordinatesInPlayableBoard(slots::SymbolType::SCATTER);
                    
                    if (sDebugBoardConfigTypes.at(sDebugBoardConfigTypesIndex) == SCATTER_3 && scatterCoordinates.size() == 3)
                    {
                        break;
                    }
                    else if (sDebugBoardConfigTypes.at(sDebugBoardConfigTypesIndex) == SCATTER_5 && scatterCoordinates.size() == 5)
                    {
                        break;
                    }
                    else if (sDebugBoardConfigTypes.at(sDebugBoardConfigTypesIndex) == COMBO_1 && numberOfCombos == 1)
                    {
                        break;
                    }
                    else if (sDebugBoardConfigTypes.at(sDebugBoardConfigTypesIndex) == COMBO_2 && numberOfCombos == 2)
                    {
                        break;
                    }
                    else if (sDebugBoardConfigTypes.at(sDebugBoardConfigTypesIndex) == COMBO_3 && numberOfCombos == 3)
                    {
                        break;
                    }
                    else if (sDebugBoardConfigTypes.at(sDebugBoardConfigTypesIndex) == COMBO_4 && numberOfCombos == 4)
                    {
                        break;
                    }
                }
                
                game.mSpinId = spinId;
                logging::Log(logging::LogType::INFO, "Magic Spin %d!", game.mSpinId);
                
                game.mBoardView->ResetBoardSymbols();
                game.mBoardModel.PopulateBoardForSpin(game.mSpinId);
                game.mBoardView->BeginSpin();
            }
            ImGui::Separator();
        }
       
        ImGui::Text("Spin Animation State: %s", game.mBoardView->GetSpinAnimationStateName().c_str());
    }
    
    ImGui::Separator();
    if (ImGui::BeginTable("Pending Symbol State", slots::BOARD_COLS))
    {
        ImGui::TableNextRow();
        for (int column = 0; column < slots::BOARD_COLS; column++)
        {
            ImGui::TableSetColumnIndex(column);
            ImGui::Text("%s", game.mBoardView ? game.mBoardView->GetPendingSymbolDataStateName(column).c_str() : "LOCKED");
        }
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("Board View", slots::BOARD_COLS))
    {
        for (int row = 0; row < slots::REEL_LENGTH; row++)
        {
            ImGui::TableNextRow();
            for (int column = 0; column < slots::BOARD_COLS; column++)
            {
                ImGui::TableSetColumnIndex(column);
                
                if (row <= 2 || row >= 6)
                {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", slots::Board::GetSymbolDebugName(game.mBoardModel.GetBoardSymbol(row, column)).c_str());
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", slots::Board::GetSymbolDebugName(game.mBoardModel.GetBoardSymbol(row, column)).c_str());
                }
            }
        }
        ImGui::EndTable();
    }
    
    ImGui::End();
    
    ImGui::Begin("Paylines", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    static int sPaylineIndex = 0;
    static float sRevealDurationSecs = 1.0f;
    static float sHiddingDurationSecs = 0.5f;
    static std::vector<std::string> sPaylines;

    if (sPaylines.empty())
    {
        for (int i = 0; i < static_cast<int>(slots::PaylineType::PAYLINE_COUNT); ++i)
        {
            sPaylines.emplace_back(PaylineView::GetPaylineName(static_cast<slots::PaylineType>(i)));
        }
    }
    
    if (ImGui::BeginCombo(" ", sPaylines.at(sPaylineIndex).c_str()))
    {
        for (auto n = 0U; n < sPaylines.size(); n++)
        {
            const bool isSelected = (sPaylineIndex == n);
            if (ImGui::Selectable(sPaylines.at(n).c_str(), isSelected))
            {
                sPaylineIndex = n;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SliderFloat("Reveal Duration(s)", &sRevealDurationSecs, 0.01f, 5.0f);
    ImGui::SliderFloat("Hiding Duration(s)", &sHiddingDurationSecs, 0.01f, 5.0f);
    if (ImGui::Button("Animate Payline"))
    {
        if (game.mBoardView)
        {
            slots::PaylineResolutionData paylineResolutionData;
            paylineResolutionData.mPayline = static_cast<slots::PaylineType>(sPaylineIndex);
            game.mBoardView->AnimatePaylineReveal(paylineResolutionData, sRevealDurationSecs, sHiddingDurationSecs);
        }
    }
    ImGui::End();
}

///------------------------------------------------------------------------------------------------

#endif
