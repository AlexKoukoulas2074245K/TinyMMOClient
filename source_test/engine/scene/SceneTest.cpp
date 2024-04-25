///------------------------------------------------------------------------------------------------
///  SceneTest.cpp
///  Predators
///
///  Created by Alex Koukoulas on 28/09/2023
///------------------------------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>

TEST(SceneOperationTests, TestBasicInsertionAndRetrieval)
{
    const strutils::StringId NAME("ABCD");
    
    scene::Scene testScene(strutils::StringId("test"));
    
    auto testSceneObject = testScene.CreateSceneObject();
    testSceneObject->mName = NAME;
    
    EXPECT_EQ(testScene.GetSceneObjectCount(), 1);
    
    auto sameTestSceneObject = testScene.FindSceneObject(NAME);
    
    EXPECT_NE(sameTestSceneObject, nullptr);
    
    EXPECT_EQ(sameTestSceneObject->mName, NAME);
}

TEST(SceneOperationTests, TestPointerValidityPostMassInsertion)
{
    const strutils::StringId NAME("ABCD");
    
    scene::Scene testScene(strutils::StringId("test"));
    
    auto testSceneObject = testScene.CreateSceneObject();
    testSceneObject->mName = NAME;
    
    EXPECT_EQ(testScene.GetSceneObjectCount(), 1);
    
    auto sameTestSceneObject = testScene.FindSceneObject(NAME);
    
    EXPECT_NE(sameTestSceneObject, nullptr);
    
    EXPECT_EQ(sameTestSceneObject->mName, NAME);
    
    for (int i = 0; i < 9999; i++)
    {
        auto so = testScene.CreateSceneObject();
    }
    
    EXPECT_EQ(testScene.GetSceneObjectCount(), 10000);
    
    sameTestSceneObject = testScene.FindSceneObject(NAME);
    
    EXPECT_NE(sameTestSceneObject, nullptr);
    
    EXPECT_EQ(testSceneObject->mName, sameTestSceneObject->mName);
    
    testSceneObject->mName = strutils::StringId("ABCDE");
    
    EXPECT_EQ(testSceneObject->mName, sameTestSceneObject->mName);
}

TEST(SceneOperationTests, TestBasicInsertionAndRemoval)
{
    const strutils::StringId NAME("ABCD");
    
    scene::Scene testScene(strutils::StringId("test"));
    
    auto testSceneObject = testScene.CreateSceneObject();
    testSceneObject->mName = NAME;
    
    EXPECT_EQ(testScene.GetSceneObjectCount(), 1);
    
    auto sameTestSceneObject = testScene.FindSceneObject(NAME);
    
    EXPECT_NE(sameTestSceneObject, nullptr);
    
    testScene.RemoveSceneObject(NAME);
    
    EXPECT_EQ(testScene.GetSceneObjectCount(), 0);
    
    EXPECT_EQ(testScene.FindSceneObject(NAME), nullptr);
}

TEST(SceneOperationTests, TestRemovalOfEmptyName)
{
    const strutils::StringId EMPTY_NAME;
    const strutils::StringId NAME("ABCD");
    
    scene::Scene testScene(strutils::StringId("test"));
    
    auto testSceneObject = testScene.CreateSceneObject();
    testSceneObject->mName = NAME;
    
    EXPECT_EQ(testScene.GetSceneObjectCount(), 1);
    
    testScene.RemoveSceneObject(EMPTY_NAME);
    
    EXPECT_EQ(testScene.GetSceneObjectCount(), 1);
    
    auto emptyNameTestSceneObject = testScene.CreateSceneObject();
    // no-op
    testSceneObject->mName = EMPTY_NAME;

    EXPECT_EQ(testScene.GetSceneObjectCount(), 2);
    
    testScene.RemoveSceneObject(EMPTY_NAME);
    
    EXPECT_EQ(testScene.GetSceneObjectCount(), 1);
}
