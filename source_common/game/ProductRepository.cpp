///------------------------------------------------------------------------------------------------
///  ProductRepository.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 04/02/2024                                                       
///------------------------------------------------------------------------------------------------

#include <game/GameSymbolicGlyphNames.h>
#include <game/ProductRepository.h>
#include <engine/CoreSystemsEngine.h>
#include <engine/resloading/DataFileResource.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/utils/BaseDataFileDeserializer.h>
#include <nlohmann/json.hpp>

///------------------------------------------------------------------------------------------------

ProductRepository& ProductRepository::GetInstance()
{
    static ProductRepository instance;
    return instance;
}

///------------------------------------------------------------------------------------------------

ProductRepository::ProductRepository()
{
}

///------------------------------------------------------------------------------------------------

const std::unordered_map<strutils::StringId, ProductDefinition, strutils::StringIdHasher> ProductRepository::GetProductDefinitions() const
{
    return mProductDefinitions;
}

///------------------------------------------------------------------------------------------------

std::vector<strutils::StringId> ProductRepository::GetRareItemProductNames() const
{
    std::vector<strutils::StringId> rareItemProductNames;
    for (const auto& productEntry: mProductDefinitions)
    {
        if (!productEntry.second.mStoryRareItemName.empty())
        {
            rareItemProductNames.push_back(productEntry.first);
        }
    }
    
    return rareItemProductNames;
}

///------------------------------------------------------------------------------------------------

const ProductDefinition& ProductRepository::GetProductDefinition(const strutils::StringId& productDefinitionName) const
{
    static ProductDefinition emptyProductDef(strutils::StringId(""), "", "", "Invalid Product", 0);
    auto foundIter = mProductDefinitions.find(productDefinitionName);
    if (foundIter != mProductDefinitions.cend())
    {
        return foundIter->second;
    }
    
    assert(false);
    return emptyProductDef;
}

///------------------------------------------------------------------------------------------------

void ProductRepository::InsertDynamicProductDefinition(const strutils::StringId& productDefinitionName, ProductDefinition&& productDefinition)
{
    mProductDefinitions.erase(productDefinitionName);
    mProductDefinitions.emplace(std::make_pair(productDefinitionName, std::move(productDefinition)));
}

///------------------------------------------------------------------------------------------------

void ProductRepository::LoadProductDefinitions()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto productDefinitionJsonResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_DATA_ROOT + "product_definitions.json", resources::DONT_RELOAD);
    const auto particlesJson =  nlohmann::json::parse(systemsEngine.GetResourceLoadingService().GetResource<resources::DataFileResource>(productDefinitionJsonResourceId).GetContents());
    
    for (const auto& productDefinitionObject: particlesJson["product_definitions"])
    {
        strutils::StringId productName = strutils::StringId(productDefinitionObject["name"].get<std::string>());
        int productPrice = productDefinitionObject["price"].get<int>();
        std::string productTexturePath = productDefinitionObject["texture_path"].get<std::string>();
        std::string shaderPath = resources::ResourceLoadingService::RES_SHADERS_ROOT + "basic.vs";
        std::string productDescription = productDefinitionObject["description"].get<std::string>();
        
        // preprocess product description
        for (const auto& symbolicNameEntry: symbolic_glyph_names::SYMBOLIC_NAMES)
        {
            strutils::StringReplaceAllOccurences("<" + symbolicNameEntry.first.GetString() + ">", std::string(1, symbolicNameEntry.second), productDescription);
        }
        
        std::string storyRareItemName = "";
        
        if (productDefinitionObject.count("shader_path"))
        {
            shaderPath = resources::ResourceLoadingService::RES_SHADERS_ROOT + productDefinitionObject["shader_path"].get<std::string>();
        }
        
        if (productDefinitionObject.count("story_rare_item_name"))
        {
            storyRareItemName = productDefinitionObject["story_rare_item_name"].get<std::string>();
        }
        
        bool unique = false;
        if (productDefinitionObject.count("unique"))
        {
            unique = productDefinitionObject["unique"].get<bool>();
        }
        
        mProductDefinitions.emplace(std::make_pair(productName, ProductDefinition(productName, productTexturePath, shaderPath, productDescription, productPrice, storyRareItemName, unique)));
    }
}

///------------------------------------------------------------------------------------------------
