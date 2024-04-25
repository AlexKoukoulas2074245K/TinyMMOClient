///------------------------------------------------------------------------------------------------
///  ProductRepository.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 04/02/2024                                                       
///------------------------------------------------------------------------------------------------

#ifndef ProductRepository_h
#define ProductRepository_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/StringUtils.h>
#include <unordered_map>
#include <variant>

///------------------------------------------------------------------------------------------------

struct ProductDefinition
{
    ProductDefinition(const strutils::StringId& productName, const std::variant<int, std::string>& productTexturePathOrCardId, const std::string& shaderPath, const std::string& description, const int price, const std::string& storyRareItemName = "", const bool unique = false)
        : mProductName(productName)
        , mProductTexturePathOrCardId(productTexturePathOrCardId)
        , mShaderPath(shaderPath)
        , mDescription(description)
        , mPrice(price)
        , mStoryRareItemName(storyRareItemName)
        , mUnique(unique)
    {
    }
    
    const strutils::StringId mProductName;
    const std::variant<int, std::string> mProductTexturePathOrCardId;
    const std::string mShaderPath;
    const std::string mDescription;
    const int mPrice;
    const std::string mStoryRareItemName;
    const bool mUnique;
};

///------------------------------------------------------------------------------------------------

class ProductRepository final
{
public:
    static ProductRepository& GetInstance();
    ~ProductRepository() = default;
    
    ProductRepository(const ProductRepository&) = delete;
    ProductRepository(ProductRepository&&) = delete;
    const ProductRepository& operator = (const ProductRepository&) = delete;
    ProductRepository& operator = (ProductRepository&&) = delete;
    
    std::vector<strutils::StringId> GetRareItemProductNames() const;
    const std::unordered_map<strutils::StringId, ProductDefinition, strutils::StringIdHasher> GetProductDefinitions() const;
    const ProductDefinition& GetProductDefinition(const strutils::StringId& productDefinitionName) const;
    
    void InsertDynamicProductDefinition(const strutils::StringId& productDefinitionName, ProductDefinition&& productDefinition);
    void LoadProductDefinitions();
    
private:
    ProductRepository();
    
private:
    std::unordered_map<strutils::StringId, ProductDefinition, strutils::StringIdHasher> mProductDefinitions;
};

///------------------------------------------------------------------------------------------------

#endif /* ProductRepository_h */
