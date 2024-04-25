///------------------------------------------------------------------------------------------------
///  CardSelectionRewardSceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 14/12/2023
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/input/IInputStateManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/resloading/MeshResource.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/SceneObjectUtils.h>
#include <engine/sound/SoundManager.h>
#include <game/AnimatedButton.h>
#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/CardTooltipController.h>
#include <game/GuiObjectManager.h>
#include <game/events/EventSystem.h>
#include <game/DataRepository.h>
#include <game/scenelogicmanagers/CardPackRewardSceneLogicManager.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId TITLE_SCENE_OBJECT_NAME = strutils::StringId("card_pack_title");
static const strutils::StringId OPEN_BUTTON_SCENE_OBJECT_NAME = strutils::StringId("open_button");
static const strutils::StringId CARD_PACK_OPENING_EFFECT_PARTICLE_NAME = strutils::StringId("card_pack_opening_sparkes");
static const strutils::StringId CARD_PACK_OPENING_EFFECT_PARTICLE_EMITTER_NAME = strutils::StringId("card_pack_opening_effect_emitter");
static const strutils::StringId DARKEN_UNIFORM_NAME = strutils::StringId("darken");
static const strutils::StringId CONTINUE_BUTTON_SCENE_OBJECT_NAME = strutils::StringId("continue_button");
static const strutils::StringId CARD_SELECTION_ANIMATION_NAME = strutils::StringId("card_selection_animation");
static const strutils::StringId CARD_PACK_REWARD_SCENE_OBJECT_NAME = strutils::StringId("card_pack_reward");

static const std::string CARD_PACK_SWIPE_SFX = "sfx_swipe";
static const std::string EXPLOSION_SFX = "sfx_explosion";
static const std::string FIREWORKS_SFX = "sfx_fireworks";
static const std::string VICTORY_SFX = "sfx_victory";

static const std::string CARD_PACK_REWARD_MESH_FILE_NAME = "card_pack_dynamic.obj";
static const std::string GOLDEN_CARD_PACK_SHADER_FILE_NAME = "card_pack_golden.vs";
static const std::string GOLDEN_CARD_PACK_TEXTURE_FILE_NAME = "card_pack_golden.png";
static const std::string NORMAL_CARD_PACK_SHADER_FILE_NAME = "basic.vs";
static const std::string NORMAL_CARD_PACK_TEXTURE_FILE_NAME = "card_pack_normal.png";
static const std::string CARD_REWARD_SCENE_OBJECT_NAME_PREFIX = "card_reward_";
static const std::string CARD_REWARD_SHADER_FILE_NAME = "card_reward.vs";
static const std::string FAMILY_STAMP_MASK_TEXTURE_FILE_NAME = "trap_mask.png";
static const std::string CARD_FAMILY_STAMP_SHADER_FILE_NAME = "card_family_stamp.vs";

static const glm::vec3 BUTTON_SCALE = {0.0005f, 0.0005f, 0.0005f};
static const glm::vec3 CONTINUE_BUTTON_POSITION = {0.0f, -0.18f, 23.2f};
static const glm::vec3 OPEN_BUTTON_POSITION = {-0.11f, -0.18f, 23.1f};
static const glm::vec3 PACK_VERTEX_GRAVITY = {0.0f, -0.00008f, 0.0f};
static const glm::vec3 CARD_PACK_INIT_POSITION = {-0.025f, -0.025f, 23.2f};
static const glm::vec3 CARD_PACK_TARGET_POSITION = {-0.025f, 0.015f, 23.2f};
static const glm::vec3 CARD_PACK_INIT_SCALE = {1/60.0f, 1/60.0f, 1/60.0f};
static const glm::vec3 CARD_PACK_TARGET_SCALE = {1.25f/60.0f, 1.25f/60.0f, 1.25f/60.0f};
static const glm::vec3 CARD_PACK_PARTICLE_EMITTER_POSITION = {0.0f, 0.0f, 23.2f};
static const glm::vec3 CARD_REWARD_INIT_SCALE = glm::vec3(0.001f, 0.001f, 2.0f);
static const glm::vec3 CARD_REWARD_DEFAULT_SCALE = glm::vec3(-0.273f, 0.2512f, 2.0f);
static const glm::vec3 CARD_REWARD_EXPANDED_SCALE = 1.25f * CARD_REWARD_DEFAULT_SCALE;
static const glm::vec3 CARD_TOOLTIP_POSITION_OFFSET = {0.0f, 0.1f, 0.0f};
static const glm::vec3 CARD_TOOLTIP_BASE_SCALE = {0.3f, 0.274f, 1/10.0f};

static const float PACK_EXPLOSION_NOISE_MAG = 0.006f;
static const float PACK_EXPLOSION_VELOCITY_MAG = 0.06f;
static const float PACK_EXPLOSION_ALPHA_REDUCTION_SPEED = 0.001f;
static const float PACK_SHAKE_STEP_DURATION = 0.01f;
static const float PACK_SHAKE_POSITION_NOISE_MAGNITUDE = 0.02f;
static const float PACK_SHAKE_SCALE_ANIMATION_DURATION_SECS = 2.0f;
static const float PACK_EXPLOSION_ALPHA_REDUCTION_ANIMATION_DURATION_SECS = 1.0f;
static const float PACK_TARGET_ROTATION = math::PI * 10.0f;
static const float PACK_PARTICLE_EMITTER_LIVE_DURATION_SECS = 6.0f;
static const float SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS = 0.5f;
static const float STAGGERED_ITEM_ALPHA_DELAY_SECS = 0.1f;
static const float CARD_REWARD_SURFACE_DELAY_SECS = 0.5f;
static const float CONTINUE_BUTTON_SNAP_TO_EDGE_FACTOR = 950000.0f;
static const float CARD_HIGHLIGHT_ANIMATION_DURATION_SECS = 0.5f;
static const float GOLDEN_CARD_CHANCE_ON_NORMAL_PACK = 0.03f;

static constexpr int PACK_CARD_REWARD_COUNT = 3;
static constexpr int PACK_MAX_SHAKE_STEPS = 100;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    game_constants::CARD_PACK_REWARD_SCENE
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    game_constants::OVERLAY_SCENE_OBJECT_NAME
};

static const std::unordered_map<CardPackType, std::string> CARD_PACK_TYPE_TO_TITLE_TEXT =
{
    { CardPackType::NORMAL, "Card Pack Reward!" },
    { CardPackType::GOLDEN, "Golden Pack Reward!" },
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& CardPackRewardSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

CardPackRewardSceneLogicManager::CardPackRewardSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

CardPackRewardSceneLogicManager::~CardPackRewardSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void CardPackRewardSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void CardPackRewardSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    mSceneState = SceneState::PENDING_PACK_OPENING;
    mCardPackShakeStepsRemaining = PACK_MAX_SHAKE_STEPS;
    mGoldenCardLightPosX = game_constants::GOLDEN_CARD_LIGHT_POS_MIN_MAX_X.s;
    
    mCardPackType = DataRepository::GetInstance().PopFrontPendingCardPack();
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(CARD_PACK_SWIPE_SFX);
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(EXPLOSION_SFX);
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(FIREWORKS_SFX);
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(VICTORY_SFX);
    
    auto cardPackReward = scene->CreateSceneObject(CARD_PACK_REWARD_SCENE_OBJECT_NAME);
    cardPackReward->mPosition = CARD_PACK_INIT_POSITION;
    cardPackReward->mScale = CARD_PACK_INIT_SCALE/10.0f;
    
    CoreSystemsEngine::GetInstance().GetResourceLoadingService().UnloadResource(resources::ResourceLoadingService::RES_MESHES_ROOT + CARD_PACK_REWARD_MESH_FILE_NAME);
    cardPackReward->mMeshResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_MESHES_ROOT + CARD_PACK_REWARD_MESH_FILE_NAME);
    cardPackReward->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + (mCardPackType == CardPackType::NORMAL ? NORMAL_CARD_PACK_TEXTURE_FILE_NAME : GOLDEN_CARD_PACK_TEXTURE_FILE_NAME));
    cardPackReward->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + (mCardPackType == CardPackType::NORMAL ? NORMAL_CARD_PACK_SHADER_FILE_NAME : GOLDEN_CARD_PACK_SHADER_FILE_NAME));
    cardPackReward->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    
    mCardTooltipController = nullptr;
    mCardRewards.clear();
    mCardRewardFamilyStamps.clear();
    CreateCardRewards(scene);
    
    std::get<scene::TextSceneObjectData>(scene->FindSceneObject(TITLE_SCENE_OBJECT_NAME)->mSceneObjectTypeData).mText = CARD_PACK_TYPE_TO_TITLE_TEXT.at(mCardPackType);
    
    // One-time card pack consolation prize
    if (DataRepository::GetInstance().GetGamesFinishedCount() == 1)
    {
        std::get<scene::TextSceneObjectData>(scene->FindSceneObject(TITLE_SCENE_OBJECT_NAME)->mSceneObjectTypeData).mText = "First Game Reward!";
    }
    
    mContinueButton = std::make_unique<AnimatedButton>
    (
        CONTINUE_BUTTON_POSITION,
        BUTTON_SCALE,
        game_constants::DEFAULT_FONT_NAME,
        "Continue",
        CONTINUE_BUTTON_SCENE_OBJECT_NAME,
        [=]()
        {
            events::EventSystem::GetInstance().DispatchEvent<events::PopSceneModalEvent>();
            mSceneState = SceneState::LEAVING_SCENE;
        },
        *scene,
        scene::SnapToEdgeBehavior::SNAP_TO_RIGHT_EDGE,
        CONTINUE_BUTTON_SNAP_TO_EDGE_FACTOR
    );
    mContinueButton->GetSceneObject()->mInvisible = false;
    mContinueButton->GetSceneObject()->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    
    mOpenButton = std::make_unique<AnimatedButton>
    (
        OPEN_BUTTON_POSITION,
        BUTTON_SCALE,
        game_constants::DEFAULT_FONT_NAME,
        "Open Pack",
        OPEN_BUTTON_SCENE_OBJECT_NAME,
        [=]()
        {
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(mOpenButton->GetSceneObject(), 0.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS, animation_flags::NONE), [=](){ mOpenButton->GetSceneObject()->mInvisible = true; });
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(cardPackReward, CARD_PACK_TARGET_POSITION, CARD_PACK_INIT_SCALE, 3.0f), [=](){});
            mSceneState = SceneState::PACK_ROTATING;
        },
        *scene
    );
    
    size_t sceneObjectIndex = 0;
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME || sceneObject->mName == CONTINUE_BUTTON_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        if (std::find_if(mCardRewards.begin(), mCardRewards.end(), [=](std::shared_ptr<CardSoWrapper> cardReward){ return cardReward->mSceneObject->mName == sceneObject->mName; }) != mCardRewards.end())
        {
            continue;
        }
        
        if (std::find_if(mCardRewardFamilyStamps.begin(), mCardRewardFamilyStamps.end(), [=](std::shared_ptr<scene::SceneObject> cardRewardFamilyStamp){ return cardRewardFamilyStamp->mName == sceneObject->mName; }) != mCardRewardFamilyStamps.end())
        {
            continue;
        }
        
        sceneObject->mInvisible = false;
        
        if (sceneObject->mName == CARD_PACK_REWARD_SCENE_OBJECT_NAME)
        {
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(sceneObject, sceneObject->mPosition, CARD_PACK_INIT_SCALE, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS, animation_flags::NONE, sceneObjectIndex * STAGGERED_ITEM_ALPHA_DELAY_SECS, math::ElasticFunction, math::TweeningMode::EASE_IN), [=](){});
            
            CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(CARD_PACK_SWIPE_SFX);
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS/5.0f, animation_flags::NONE, sceneObjectIndex++ * STAGGERED_ITEM_ALPHA_DELAY_SECS), [=](){});
    }
    RegisterForEvents();
}

///------------------------------------------------------------------------------------------------

void CardPackRewardSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> scene)
{
    static float time = 0.0f;
    time += dtMillis * 0.001f;
    
    auto cardPackReward = scene->FindSceneObject(strutils::StringId(CARD_PACK_REWARD_SCENE_OBJECT_NAME));
    cardPackReward->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
    
    for (auto& cardReward: mCardRewards)
    {
        cardReward->mSceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
        cardReward->mSceneObject->mShaderFloatUniformValues[game_constants::LIGHT_POS_X_UNIFORM_NAME] = mGoldenCardLightPosX;
    }
    
    switch (mSceneState)
    {
        case SceneState::PENDING_PACK_OPENING:
        {
            cardPackReward->mRotation.y = math::Sinf(time);
            mOpenButton->Update(dtMillis);
        } break;
            
        case SceneState::PACK_ROTATING:
        {
            cardPackReward->mRotation.y += dtMillis * 0.01f;
            if (cardPackReward->mRotation.y >= PACK_TARGET_ROTATION)
            {
                cardPackReward->mRotation.y = PACK_TARGET_ROTATION;
                
                // Tween card pack scale up a bit
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(cardPackReward->mScale.x, CARD_PACK_TARGET_SCALE.x, PACK_SHAKE_SCALE_ANIMATION_DURATION_SECS, animation_flags::NONE), [=](){});
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(cardPackReward->mScale.y, CARD_PACK_TARGET_SCALE.y, PACK_SHAKE_SCALE_ANIMATION_DURATION_SECS, animation_flags::NONE), [=](){});
                
                // Start card pack shaking
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>
                (
                    cardPackReward,
                    glm::vec3(CARD_PACK_TARGET_POSITION.x + math::RandomFloat(-PACK_SHAKE_POSITION_NOISE_MAGNITUDE, PACK_SHAKE_POSITION_NOISE_MAGNITUDE), CARD_PACK_TARGET_POSITION.y + math::RandomFloat(-PACK_SHAKE_POSITION_NOISE_MAGNITUDE, PACK_SHAKE_POSITION_NOISE_MAGNITUDE), CARD_PACK_TARGET_POSITION.z),
                    CARD_PACK_INIT_SCALE,
                    PACK_SHAKE_STEP_DURATION,
                    animation_flags::IGNORE_SCALE
                ), [=](){ CardPackShakeStep(scene); });
                
                PreparePackVertexVelocities(scene);
                mSceneState = SceneState::PACK_SHAKING;
            }
            
        } break;
        
        case SceneState::PACK_EXPLODING:
        {
            UpdatePackVertices(dtMillis, scene);
            
            cardPackReward->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = math::Max(0.0f, cardPackReward->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] - PACK_EXPLOSION_ALPHA_REDUCTION_SPEED * dtMillis);
            if (cardPackReward->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] <= 0.0f)
            {
                mSceneState = SceneState::CARD_REWARDS_INSPECTION;
            }
        } break;
            
        case SceneState::CARD_REWARDS_INSPECTION:
        {
            auto& inputStateManager = CoreSystemsEngine::GetInstance().GetInputStateManager();
            auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
            
            auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(scene->GetCamera().GetViewMatrix(), scene->GetCamera().GetProjMatrix());
            
            bool createdTooltipThisFrame = false;
            for (auto i = 0U; i < mCardRewards.size(); ++i)
            {
                auto cardSoWrapper = mCardRewards[i];
                auto sceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*cardSoWrapper->mSceneObject);
                bool cursorInSceneObject = math::IsPointInsideRectangle(sceneObjectRect.bottomLeft, sceneObjectRect.topRight, worldTouchPos);
                
#if defined(MOBILE_FLOW)
                if (cursorInSceneObject && inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON))
                {
                    if (cardSoWrapper->mState == CardSoState::IDLE)
                    {
                        cardSoWrapper->mState = CardSoState::HIGHLIGHTED;
                        
                        std::vector<std::shared_ptr<scene::SceneObject>> cardSceneObjectGroup = {cardSoWrapper->mSceneObject, mCardRewardFamilyStamps[i]};
                        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleGroupAnimation>(cardSceneObjectGroup, cardSoWrapper->mSceneObject->mPosition, CARD_REWARD_EXPANDED_SCALE, CARD_HIGHLIGHT_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [=](){}, CARD_SELECTION_ANIMATION_NAME);
                    }
                    
                    if (cardSoWrapper->mCardData.IsSpell())
                    {
                        DestroyCardTooltip(scene);
                        CreateCardTooltip(cardSoWrapper->mSceneObject->mPosition, cardSoWrapper->mCardData.mCardEffectTooltip, i, scene);
                        createdTooltipThisFrame = true;
                    }
                }
                else if (!cursorInSceneObject && inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON))
                {
                    if (cardSoWrapper->mState == CardSoState::HIGHLIGHTED)
                    {
                        if (!createdTooltipThisFrame)
                        {
                            DestroyCardTooltip(scene);
                        }
                        
                        cardSoWrapper->mState = CardSoState::IDLE;
                        
                        std::vector<std::shared_ptr<scene::SceneObject>> cardSceneObjectGroup = {cardSoWrapper->mSceneObject, mCardRewardFamilyStamps[i]};
                        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleGroupAnimation>(cardSceneObjectGroup, cardSoWrapper->mSceneObject->mPosition, CARD_REWARD_DEFAULT_SCALE, CARD_HIGHLIGHT_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [=](){}, CARD_SELECTION_ANIMATION_NAME);
                    }
                }
#else
                if (cursorInSceneObject && cardSoWrapper->mState == CardSoState::IDLE && cardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] >= 1.0f)
                {
                    cardSoWrapper->mState = CardSoState::HIGHLIGHTED;
                    std::vector<std::shared_ptr<scene::SceneObject>> cardSceneObjectGroup = {cardSoWrapper->mSceneObject, mCardRewardFamilyStamps[i]};
                    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleGroupAnimation>(cardSceneObjectGroup, cardSoWrapper->mSceneObject->mPosition, CARD_REWARD_EXPANDED_SCALE, CARD_HIGHLIGHT_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [=](){}, CARD_SELECTION_ANIMATION_NAME);
                    
                    if (cardSoWrapper->mCardData.IsSpell())
                    {
                        DestroyCardTooltip(scene);
                        CreateCardTooltip(cardSoWrapper->mSceneObject->mPosition, cardSoWrapper->mCardData.mCardEffectTooltip, i, scene);
                        createdTooltipThisFrame = true;
                    }
                }
                else if (!cursorInSceneObject && cardSoWrapper->mState == CardSoState::HIGHLIGHTED)
                {
                    if (!createdTooltipThisFrame)
                    {
                        DestroyCardTooltip(scene);
                    }
                    
                    cardSoWrapper->mState = CardSoState::IDLE;
            
                    std::vector<std::shared_ptr<scene::SceneObject>> cardSceneObjectGroup = {cardSoWrapper->mSceneObject, mCardRewardFamilyStamps[i]};
                    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleGroupAnimation>(cardSceneObjectGroup, cardSoWrapper->mSceneObject->mPosition, CARD_REWARD_DEFAULT_SCALE, CARD_HIGHLIGHT_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [=](){}, CARD_SELECTION_ANIMATION_NAME);
                }
#endif
            }
            
            mContinueButton->Update(dtMillis);
            
            if (mCardTooltipController)
            {
                mCardTooltipController->Update(dtMillis);
            }
        } break;
            
        default: break;
    }
}

///------------------------------------------------------------------------------------------------

void CardPackRewardSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
{
    DestroyCardTooltip(scene);
    scene->RemoveSceneObject(CARD_PACK_OPENING_EFFECT_PARTICLE_EMITTER_NAME);
    
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS), [=]()
        {
            sceneObject->mInvisible = true;
            
            if (sceneObject->mName != TITLE_SCENE_OBJECT_NAME)
            {
                scene->RemoveSceneObject(sceneObject->mName);
            }
        });
    }
    
    events::EventSystem::GetInstance().UnregisterAllEventsForListener(this);
    mCardRewards.clear();
    mCardRewardFamilyStamps.clear();
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> CardPackRewardSceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------

void CardPackRewardSceneLogicManager::RegisterForEvents()
{
    auto& eventSystem = events::EventSystem::GetInstance();
    
    eventSystem.RegisterForEvent<events::WindowResizeEvent>(this, &CardPackRewardSceneLogicManager::OnWindowResize);
}

///------------------------------------------------------------------------------------------------

void CardPackRewardSceneLogicManager::OnWindowResize(const events::WindowResizeEvent&)
{
    CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::CARD_PACK_REWARD_SCENE)->RecalculatePositionOfEdgeSnappingSceneObjects();
}

///------------------------------------------------------------------------------------------------

void CardPackRewardSceneLogicManager::PreparePackVertexVelocities(std::shared_ptr<scene::Scene> scene)
{
    auto cardPackReward = scene->FindSceneObject(strutils::StringId(CARD_PACK_REWARD_SCENE_OBJECT_NAME));
    auto& cardPackMesh = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::MeshResource>(cardPackReward->mMeshResourceId);
    
    cardPackMesh.ApplyDirectTransformToData([=](resources::MeshResource::MeshData& meshData)
    {
        mCardPackVertexVelocities.resize(meshData.mVertices.size());
        
        for (int i = 0; i < meshData.mVertices.size(); ++i)
        {
            auto randomVelocityOffset = glm::vec3(math::RandomFloat(-PACK_EXPLOSION_NOISE_MAG, PACK_EXPLOSION_NOISE_MAG), math::RandomFloat(-PACK_EXPLOSION_NOISE_MAG, PACK_EXPLOSION_NOISE_MAG), 0.0f);
            if (math::Abs(meshData.mNormals[i].z) > 0.8)
            {
                
                mCardPackVertexVelocities[i] = glm::normalize(meshData.mVertices[i]) * PACK_EXPLOSION_VELOCITY_MAG + randomVelocityOffset;
                mCardPackVertexVelocities[i + 1] = glm::normalize(meshData.mVertices[i]) * PACK_EXPLOSION_VELOCITY_MAG + randomVelocityOffset;
                mCardPackVertexVelocities[i + 2] = glm::normalize(meshData.mVertices[i]) * PACK_EXPLOSION_VELOCITY_MAG + randomVelocityOffset;

                i += 2;
            }
            else
            {
                mCardPackVertexVelocities[i] += glm::normalize(meshData.mNormals[i]) * PACK_EXPLOSION_VELOCITY_MAG + randomVelocityOffset;
            }
        }
    });
}

///------------------------------------------------------------------------------------------------

void CardPackRewardSceneLogicManager::UpdatePackVertices(const float dtMillis, std::shared_ptr<scene::Scene> scene)
{
    auto cardPackReward = scene->FindSceneObject(strutils::StringId(CARD_PACK_REWARD_SCENE_OBJECT_NAME));
    auto& cardPackMesh = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::MeshResource>(cardPackReward->mMeshResourceId);
    
    cardPackMesh.ApplyDirectTransformToData([=](resources::MeshResource::MeshData& meshData)
    {
        for (int i = 0; i < meshData.mVertices.size(); ++i)
        {
            float oldZ = meshData.mVertices[i].z;
            mCardPackVertexVelocities[i] += PACK_VERTEX_GRAVITY * dtMillis;
            meshData.mVertices[i] += mCardPackVertexVelocities[i] * dtMillis;
            meshData.mVertices[i].z = oldZ;
        }
    });
}

///------------------------------------------------------------------------------------------------

void CardPackRewardSceneLogicManager::CardPackShakeStep(std::shared_ptr<scene::Scene> scene)
{
    auto cardPackReward = scene->FindSceneObject(strutils::StringId(CARD_PACK_REWARD_SCENE_OBJECT_NAME));
    
    if (mCardPackShakeStepsRemaining-- == 0)
    {
        mSceneState = SceneState::PACK_EXPLODING;
        
        // Create sparkles particle effect
        CoreSystemsEngine::GetInstance().GetParticleManager().CreateParticleEmitterAtPosition
        (
            CARD_PACK_OPENING_EFFECT_PARTICLE_NAME,
            CARD_PACK_PARTICLE_EMITTER_POSITION,
            *scene,
            CARD_PACK_OPENING_EFFECT_PARTICLE_EMITTER_NAME
        );
        
        // Stop particle emitter after some time
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(PACK_PARTICLE_EMITTER_LIVE_DURATION_SECS), [=]()
        {
            CoreSystemsEngine::GetInstance().GetParticleManager().RemoveParticleEmitterFlag(particle_flags::CONTINUOUS_PARTICLE_GENERATION, CARD_PACK_OPENING_EFFECT_PARTICLE_EMITTER_NAME, *scene);
        });
        
        // Fade out card pack vertices
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(cardPackReward, 0.0f, PACK_EXPLOSION_ALPHA_REDUCTION_ANIMATION_DURATION_SECS, animation_flags::NONE), [=]()
        {
            cardPackReward->mInvisible = true;
        });
        
        // Fade in card rewards
        for (auto i = 0; i < mCardRewards.size(); ++i)
        {
            mCardRewards[i]->mSceneObject->mInvisible = false;
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(mCardRewards[i]->mSceneObject, mCardRewards[i]->mSceneObject->mPosition, CARD_REWARD_DEFAULT_SCALE, CARD_REWARD_SURFACE_DELAY_SECS, animation_flags::NONE, CARD_REWARD_SURFACE_DELAY_SECS + i * CARD_REWARD_SURFACE_DELAY_SECS, math::ElasticFunction, math::TweeningMode::EASE_IN), [=](){});
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(mCardRewards[i]->mSceneObject, 1.0f, CARD_REWARD_SURFACE_DELAY_SECS, animation_flags::NONE, CARD_REWARD_SURFACE_DELAY_SECS + i * CARD_REWARD_SURFACE_DELAY_SECS), [=](){});
            
            mCardRewardFamilyStamps[i]->mInvisible = false;
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(mCardRewardFamilyStamps[i], 1.0f, CARD_REWARD_SURFACE_DELAY_SECS, animation_flags::NONE, CARD_REWARD_SURFACE_DELAY_SECS + (i + 1) * CARD_REWARD_SURFACE_DELAY_SECS), [=](){});
        }
        
        // Start a light ray in case of golden cards
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mGoldenCardLightPosX, game_constants::GOLDEN_CARD_LIGHT_POS_MIN_MAX_X.t, 1.0f, animation_flags::NONE, CARD_REWARD_SURFACE_DELAY_SECS + (mCardRewards.size() + 1) * CARD_REWARD_SURFACE_DELAY_SECS), [](){});
        
        // Fade in continue button
        mContinueButton->GetSceneObject()->mInvisible = false;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(mContinueButton->GetSceneObject(), 1.0f, CARD_REWARD_SURFACE_DELAY_SECS, animation_flags::NONE), [=]()
        {
        });
        
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(EXPLOSION_SFX);
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(FIREWORKS_SFX);
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(VICTORY_SFX);
    }
    else
    {
        // Recursively create more shake steps
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>
        (
            cardPackReward,
            glm::vec3(CARD_PACK_TARGET_POSITION.x + math::RandomFloat(-PACK_SHAKE_POSITION_NOISE_MAGNITUDE, PACK_SHAKE_POSITION_NOISE_MAGNITUDE), CARD_PACK_TARGET_POSITION.y + math::RandomFloat(-PACK_SHAKE_POSITION_NOISE_MAGNITUDE, PACK_SHAKE_POSITION_NOISE_MAGNITUDE), CARD_PACK_TARGET_POSITION.z),
            cardPackReward->mScale,
            PACK_SHAKE_STEP_DURATION,
            animation_flags::IGNORE_SCALE
        ), [=](){ CardPackShakeStep(scene); });
    }
}

///------------------------------------------------------------------------------------------------

void CardPackRewardSceneLogicManager::CreateCardRewards(std::shared_ptr<scene::Scene> scene)
{
    math::SetControlSeed(DataRepository::GetInstance().GetNextCardPackSeed());
    
    auto cardRewardPool = CardDataRepository::GetInstance().GetCardPackLockedCardRewardsPool();
    auto unlockedCardIds = DataRepository::GetInstance().GetUnlockedCardIds();
    auto unlockedGoldenCardIds = DataRepository::GetInstance().GetGoldenCardIdMap();
    auto newCardIds = DataRepository::GetInstance().GetNewCardIds();
    
    // For golden packs the pool includes unlocked cards (that have not had their golden counterparts won yet)
    if (mCardPackType == CardPackType::GOLDEN)
    {
        std::copy(unlockedCardIds.begin(), unlockedCardIds.end(), std::back_inserter(cardRewardPool));
        for (auto iter = cardRewardPool.begin(); iter != cardRewardPool.end();)
        {
            if (unlockedGoldenCardIds.count(*iter))
            {
                iter = cardRewardPool.erase(iter);
            }
            else
            {
                iter++;
            }
        }
    }
    
    while (cardRewardPool.size() < PACK_CARD_REWARD_COUNT)
    {
        auto randomUnlockedCardIndex = math::ControlledRandomInt() % unlockedCardIds.size();
        
        if (std::find(cardRewardPool.begin(), cardRewardPool.end(), unlockedCardIds[randomUnlockedCardIndex]) != cardRewardPool.end())
        {
            continue;
        }
        
        cardRewardPool.push_back(unlockedCardIds[randomUnlockedCardIndex]);
    }
    
    for (size_t i = 0; i < PACK_CARD_REWARD_COUNT; ++i)
    {
        auto randomCardIndex = math::ControlledRandomInt() % cardRewardPool.size();
        auto cardData = CardDataRepository::GetInstance().GetCardData(cardRewardPool[randomCardIndex], game_constants::LOCAL_PLAYER_INDEX);
        bool isGolden = mCardPackType == CardPackType::NORMAL ? (math::ControlledRandomFloat() < GOLDEN_CARD_CHANCE_ON_NORMAL_PACK) : true;
        
        mCardRewards.push_back(card_utils::CreateCardSoWrapper(&cardData, glm::vec3(-0.2f + 0.17 * i, -0.0f, 23.2f), CARD_REWARD_SCENE_OBJECT_NAME_PREFIX + std::to_string(i), CardOrientation::FRONT_FACE, isGolden ? CardRarity::GOLDEN : CardRarity::NORMAL, true, false, true, {}, {}, *scene));
        mCardRewards.back()->mSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        mCardRewards.back()->mSceneObject->mScale = CARD_REWARD_INIT_SCALE;
        mCardRewards.back()->mSceneObject->mShaderBoolUniformValues[DARKEN_UNIFORM_NAME] = false;
        mCardRewards.back()->mSceneObject->mInvisible = true;
        mCardRewards.back()->mSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + CARD_REWARD_SHADER_FILE_NAME);
        
        // Create card family stamp
        if (cardData.mCardFamily == game_constants::RODENTS_FAMILY_NAME ||
            cardData.mCardFamily == game_constants::DINOSAURS_FAMILY_NAME ||
            cardData.mCardFamily == game_constants::INSECTS_FAMILY_NAME)
        {
            mCardRewardFamilyStamps.push_back(scene->CreateSceneObject(strutils::StringId(CARD_REWARD_SCENE_OBJECT_NAME_PREFIX + "family_stamp_" + std::to_string(i))));
            mCardRewardFamilyStamps.back()->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + game_constants::CARD_FAMILY_NAMES_TO_TEXTURES.at(cardData.mCardFamily));
            mCardRewardFamilyStamps.back()->mEffectTextureResourceIds[0] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + FAMILY_STAMP_MASK_TEXTURE_FILE_NAME);
            mCardRewardFamilyStamps.back()->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + CARD_FAMILY_STAMP_SHADER_FILE_NAME);
            mCardRewardFamilyStamps.back()->mScale.x = mCardRewardFamilyStamps.back()->mScale.y = game_constants::IN_GAME_CARD_PROPERTY_ICON_SCALE;
            mCardRewardFamilyStamps.back()->mPosition = mCardRewards.back()->mSceneObject->mPosition;
            mCardRewardFamilyStamps.back()->mPosition.x -= 0.008f;
            mCardRewardFamilyStamps.back()->mPosition.y -= 0.06f;
            mCardRewardFamilyStamps.back()->mPosition.z += game_constants::CARD_COMPONENT_Z_OFFSET;
            mCardRewardFamilyStamps.back()->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            mCardRewardFamilyStamps.back()->mInvisible = true;
        }
        
        if (std::find(unlockedCardIds.begin(), unlockedCardIds.end(), cardData.mCardId) == unlockedCardIds.end())
        {
            unlockedCardIds.push_back(cardData.mCardId);
            
            if (std::find(newCardIds.begin(), newCardIds.end(), cardData.mCardId) == newCardIds.end())
            {
                newCardIds.push_back(cardData.mCardId);
            }
        }
        
        if (isGolden && !unlockedGoldenCardIds.count(cardData.mCardId))
        {
            DataRepository::GetInstance().SetGoldenCardMapEntry(cardData.mCardId, true);
            
            if (std::find(newCardIds.begin(), newCardIds.end(), cardData.mCardId) == newCardIds.end())
            {
                newCardIds.push_back(cardData.mCardId);
            }
        }
        
        cardRewardPool.erase(cardRewardPool.begin() + randomCardIndex);
    }
    
    DataRepository::GetInstance().SetNewCardIds(newCardIds);
    DataRepository::GetInstance().SetUnlockedCardIds(unlockedCardIds);
    DataRepository::GetInstance().SetNextCardPackSeed(math::GetControlSeed());
    DataRepository::GetInstance().FlushStateToFile();
}

///------------------------------------------------------------------------------------------------

void CardPackRewardSceneLogicManager::CreateCardTooltip(const glm::vec3& cardOriginPostion, const std::string& tooltipText, const size_t cardIndex, std::shared_ptr<scene::Scene> scene)
{
    bool shouldBeHorFlipped = cardIndex > 1;
    mCardTooltipController = std::make_unique<CardTooltipController>
    (
        cardOriginPostion + CARD_TOOLTIP_POSITION_OFFSET,
        CARD_TOOLTIP_BASE_SCALE,
        tooltipText,
        false,
        shouldBeHorFlipped,
        false,
        *scene
    );
}

///------------------------------------------------------------------------------------------------

void CardPackRewardSceneLogicManager::DestroyCardTooltip(std::shared_ptr<scene::Scene> scene)
{
    if (mCardTooltipController)
    {
        for (auto sceneObject: mCardTooltipController->GetSceneObjects())
        {
            scene->RemoveSceneObject(sceneObject->mName);
        }
    }
    
    mCardTooltipController = nullptr;
}

