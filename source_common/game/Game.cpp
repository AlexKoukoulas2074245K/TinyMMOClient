///------------------------------------------------------------------------------------------------
///  Game.cpp                                                                                        
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 19/09/2023
///------------------------------------------------------------------------------------------------

#include <bitset>
#include <engine/CoreSystemsEngine.h>
#include <engine/input/IInputStateManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/Camera.h>
#include <engine/rendering/CommonUniforms.h>
#include <engine/rendering/Fonts.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/rendering/RenderingUtils.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/resloading/ImageSurfaceResource.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/scene/SceneObjectUtils.h>
#include <engine/sound/SoundManager.h>
#include <engine/utils/BaseDataFileDeserializer.h>
#include <engine/utils/Date.h>
#include <engine/utils/Logging.h>
#include <engine/utils/FileUtils.h>
#include <engine/utils/OSMessageBox.h>
#include <engine/utils/PlatformMacros.h>
#include <enet/enet.h>
#include <fstream>
#include <game/AnimatedButton.h>
#include <game/Game.h>
#include <game/events/EventSystem.h>
#include <net_common/NetworkMessages.h>
#include <mutex>
#include <SDL.h>

//#define ALLOW_OFFLINE_PLAY
#if defined(MOBILE_FLOW)
#include <platform_specific/IOSUtils.h>
#endif

#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleUtils.h>
#elif defined(WINDOWS)
#include <platform_utilities/WindowsUtils.h>
#endif

///------------------------------------------------------------------------------------------------

Game::Game(const int argc, char** argv)
{
    if (argc > 0)
    {
        logging::Log(logging::LogType::INFO, "Initializing from CWD : %s", argv[0]);
    }
    
#if defined(MACOS) || defined(MOBILE_FLOW)
    apple_utils::SetAssetFolder();
#endif
    
    CoreSystemsEngine::GetInstance().Start([&](){ Init(); }, [&](const float dtMillis){ Update(dtMillis); }, [&](){ ApplicationMovedToBackground(); }, [&](){ WindowResize(); }, [&](){ CreateDebugWidgets(); }, [&](){ OnOneSecondElapsed(); });
}

///------------------------------------------------------------------------------------------------

Game::~Game(){}

///------------------------------------------------------------------------------------------------
static ENetHost* sClient;
static ENetPeer* sPeer;

void Game::Init()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    systemsEngine.GetFontRepository().LoadFont(game_constants::DEFAULT_FONT_NAME.GetString(), resources::ResourceReloadMode::DONT_RELOAD);
    systemsEngine.GetSoundManager().SetAudioEnabled(false);
    
    enet_initialize();
    atexit(enet_deinitialize);
    
    sClient = enet_host_create(nullptr, 1, 2, 0, 0);

    ENetAddress address{};
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 7777;

    sPeer = enet_host_connect(sClient, &address, 2, 0);
    if (!sPeer)
    {
        logging::Log(logging::LogType::ERROR, "Failed to connect");
        return;
    }

    ENetEvent event;
    if (enet_host_service(sClient, &event, 5000) <= 0 ||
        event.type != ENET_EVENT_TYPE_CONNECT)
    {
        logging::Log(logging::LogType::ERROR, "Connection failed");
    }

    logging::Log(logging::LogType::INFO, "Connected to server");
    
    auto scene = systemsEngine.GetSceneManager().CreateScene(game_constants::WORLD_SCENE_NAME);
    scene->GetCamera().SetZoomFactor(50.0f);
    scene->SetLoaded(true);
}

///------------------------------------------------------------------------------------------------

void Game::Update(const float dtMillis)
{
    // Fake movement
    MoveMessage move{};
    move.playerId = 0;
    move.position = { std::cos(dtMillis), std::sin(dtMillis) };
    move.velocity = { 0.0f, 0.0f };
    
    ENetPacket* movePacket = enet_packet_create(
                                                &move,
                                                sizeof(move),
                                                0 // UNRELIABLE
                                                );
    
    enet_peer_send(sPeer, 0, movePacket);
    
    // Occasionally send reliable event
    if (static_cast<int>(dtMillis) % 5 == 0)
    {
        AttackMessage atk{};
        atk.attackerId = 0;
        
        ENetPacket* atkPacket = enet_packet_create(
                                                   &atk,
                                                   sizeof(atk),
                                                   ENET_PACKET_FLAG_RELIABLE
                                                   );
        
        enet_peer_send(sPeer, 1, atkPacket);
    }

    ENetEvent event;
    while (enet_host_service(sClient, &event, 0) > 0)
    {
        if (event.type == ENET_EVENT_TYPE_RECEIVE)
        {
            auto type = static_cast<MessageType>(event.packet->data[0]);
            if (type == MessageType::SnapshotMessage)
            {
                auto* snap = reinterpret_cast<SnapshotMessage*>(event.packet->data);
                logging::Log(logging::LogType::INFO, "Snapshot: Player %d at %.6f,%.6f", snap->playerId, snap->position.x, snap->position.y);
            }
            
            enet_packet_destroy(event.packet);
        }
    }
    
    enet_host_flush(sClient);
}

///------------------------------------------------------------------------------------------------

void Game::ApplicationMovedToBackground()
{
}

///------------------------------------------------------------------------------------------------

void Game::OnOneSecondElapsed()
{
}

///------------------------------------------------------------------------------------------------

void Game::WindowResize()
{
}

///------------------------------------------------------------------------------------------------

#if defined(USE_IMGUI)
void Game::CreateDebugWidgets()
{
}
#else
void Game::CreateDebugWidgets()
{
}
#endif

///------------------------------------------------------------------------------------------------
