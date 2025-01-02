#include "UI/CartographLayerToggleItemWidget.h"

#include "CartographGameInstanceModule.h"


void UCartographLayerToggleItemWidget::Initialize_Native(const FName& MainCategory, const FName& SubCategory)
{
    this->MainCategory = MainCategory;
    this->SubCategory = SubCategory;
}


void UCartographLayerToggleItemWidget::OnClicked(bool IsChecked) const
{
    if (SubCategory.IsNone())
    {
        if (!IsChecked)
        {
            GameInstanceModule->RuntimeConfig.DisabledLayerMainCategory.Add(MainCategory);
        }
        else
        {
            GameInstanceModule->RuntimeConfig.DisabledLayerMainCategory.Remove(MainCategory);
        }
    }
    else
    {
	    TSet<FName>& SubCategoryData = GameInstanceModule->RuntimeConfig.DisabledLayerSubCategory.FindOrAdd(MainCategory);
        if (!IsChecked)
        {
            SubCategoryData.Add(SubCategory);
        }
        else
        {
            SubCategoryData.Remove(SubCategory);
        }
    }
    GameInstanceModule->OnLayerConfigChanged();
}


bool UCartographLayerToggleItemWidget::GetInitialStatus() const
{
    if (SubCategory.IsNone())
    {
        return !GameInstanceModule->RuntimeConfig.DisabledLayerMainCategory.Contains(MainCategory);
    }
    else
    {
        if (const TSet<FName>* SubCategories = GameInstanceModule->RuntimeConfig.DisabledLayerSubCategory.Find(MainCategory))
        {
            return !SubCategories->Contains(SubCategory);
        }
    }
    return true;
}


void UCartographLayerToggleItemWidget::SelectOrDeselectAll(bool DoSelect)
{
    GameInstanceModule->FillBuildLayerDataCache();

    if (SubCategory.IsNone())
    {
        for (const auto& [ClassHash, LayerData] : GameInstanceModule->BuildLayerDataMapCache)
        {
            if (LayerData->MainCategoryCache != MainCategory)
            {
                continue;
            }

            if (DoSelect)
            {
                GameInstanceModule->RuntimeConfig.DisabledLayerBuildable.Remove(ClassHash);
            }
            else
            {
                GameInstanceModule->RuntimeConfig.DisabledLayerBuildable.Add(ClassHash);
            }
        }
    }
    else
    {
        for (const auto& [ClassHash, LayerData] : GameInstanceModule->BuildLayerDataMapCache)
        {
            if (LayerData->MainCategoryCache != MainCategory || LayerData->SubCategoryCache != SubCategory)
            {
                continue;
            }

            if (DoSelect)
            {
                GameInstanceModule->RuntimeConfig.DisabledLayerBuildable.Remove(ClassHash);
            }
            else
            {
                GameInstanceModule->RuntimeConfig.DisabledLayerBuildable.Add(ClassHash);
            }
        }
    }
    GameInstanceModule->OnLayerConfigChanged();
}
