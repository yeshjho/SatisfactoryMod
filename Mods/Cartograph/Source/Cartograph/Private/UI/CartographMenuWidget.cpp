#include "UI/CartographMenuWidget.h"

#include "PanelWidget.h"

#include "FGBuildable.h"
#include "FGBuildingDescriptor.h"
#include "FGRecipeManager.h"

#include "CartographGameInstanceModule.h"
#include "CartographLayerToggleItemWidget.h"
#include "CartographMenuCategoryWidget.h"
#include "CartographMenuLayerItemWidget.h"


void UCartographMenuWidget::InitializeHeadings(UPanelWidget* Panel)
{
    Algo::SortBy(HeadingData, &FHeadingData::Priority);
    for (const auto& [Name, DisplayName, _] : HeadingData)
    {
        auto* CategoryWidget = CreateWidget<UCartographMenuCategoryWidget>(this, CategoryWidgetType);
        CategoryWidget->Initialize(ECategoryType::Heading, DisplayName);
        Panel->AddChild(CategoryWidget);
        MenuItemHierarchy.Add(Name, FHeadingItem{ 
            DisplayName.ToString(),
			{},
            CategoryWidget,
        });
    }
}


void UCartographMenuWidget::InitializeLayers()
{
    FHeadingItem& LayerHeading = *MenuItemHierarchy.Find("Layers");

    Algo::SortBy(GameInstanceModule->LayerCategories, &FLayerCategoryData::Priority);
    for (FLayerCategoryData& CategoryData : GameInstanceModule->LayerCategories)
    {
        Algo::SortBy(CategoryData.SubCategories, &FLayerSubCategoryData::Priority);

        auto* CategoryWidget = CreateWidget<UCartographMenuCategoryWidget>(this, CategoryWidgetType);
        CategoryWidget->Initialize(ECategoryType::MainCategory, CategoryData.DisplayName);
        LayerHeading.CategoryWidget->AddCategory(CategoryWidget);
        auto* ToggleItemWidget = CreateWidget<UCartographLayerToggleItemWidget>(this, CategoryLayerToggleItemWidgetType);
        ToggleItemWidget->Initialize_Native(this, CategoryData.Name, FName{});
        CategoryWidget->AddItem(ToggleItemWidget, true);
        FMainCategoryItem MainCategoryItem{
            CategoryData.DisplayName.ToString(),
			{},
            CategoryWidget,
        };

        for (FLayerSubCategoryData& SubCategoryData : CategoryData.SubCategories)
        {
            auto* SubCategoryWidget = CreateWidget<UCartographMenuCategoryWidget>(this, CategoryWidgetType);
            SubCategoryWidget->Initialize(ECategoryType::SubCategory, SubCategoryData.DisplayName);
            CategoryWidget->AddCategory(SubCategoryWidget);
            auto* ToggleItemWidgetSub = CreateWidget<UCartographLayerToggleItemWidget>(this, CategoryLayerToggleItemWidgetType);
            ToggleItemWidgetSub->Initialize_Native(this, CategoryData.Name, SubCategoryData.Name);
            SubCategoryWidget->AddItem(ToggleItemWidgetSub, true);
            MainCategoryItem.SubCategories.Add(SubCategoryData.Name, FSubCategoryItem{
                .DisplayName = SubCategoryData.DisplayName.ToString(),
                .CategoryWidget = SubCategoryWidget,
            });
        }

        LayerHeading.MainCategories.Add(CategoryData.Name, std::move(MainCategoryItem));
    }

    const AFGRecipeManager* RecipeManager = AFGRecipeManager::Get(UCartographGameInstanceModule::Instance->GetWorld());
    CARTO_LOG_ERROR_RETURN_IF_NULL(RecipeManager);
    const auto& BuildableClassRedirectMap = UCartographGameInstanceModule::Instance->BuildableClassRedirectMap;
    for (const auto& [BuildableClass, _] : UCartographGameInstanceModule::Instance->ClassPtrToClassIDMap)
    {
	    if (!BuildableClass)
	    {
            continue;
	    }

        if (BuildableClassRedirectMap.Contains(BuildableClass.Get()))
	    {
		    continue;
	    }

        const uint32* ClassHash = UCartographGameInstanceModule::Instance->ClassPtrToClassIDMap.Find(BuildableClass);
        if (!ClassHash)
        {
            UE_LOG(LogCartograph, Error, TEXT("Can't find hash for %s"), *BuildableClass->GetName());
            continue;
        }

        const FBuildLayerData* LayerData = UCartographGameInstanceModule::Instance->GetBuildLayerData(*ClassHash);
        if (!LayerData)
        {
            UE_LOG(LogCartograph, Warning, TEXT("Can't find layer data for %s"), *BuildableClass->GetName());
            continue;
        }

        const TSubclassOf<UFGBuildingDescriptor> Descriptor = RecipeManager->FindBuildingDescriptorByClass(BuildableClass);
        if (!Descriptor)
        {
            UE_LOG(LogCartograph, Warning, TEXT("Can't find descriptor for %s"), *BuildableClass->GetName());
            continue;
        }
        const FText BuildingName = UFGItemDescriptor::GetItemName(Descriptor);
        UTexture2D* Icon = UFGItemDescriptor::GetSmallIcon(Descriptor);

        FMainCategoryItem* MainCategoryItem = LayerHeading.MainCategories.Find(LayerData->MainCategoryCache);
        if (!MainCategoryItem)
        {
            UE_LOG(LogCartograph, Warning, TEXT("Can't find main category %s"), *LayerData->MainCategoryCache.ToString());
            continue;
        }

        if (LayerData->SubCategoryCache.IsNone())
        {
            auto* ItemWidget = CreateWidget<UCartographMenuLayerItemWidget>(this, CategoryLayerItemWidgetType);
            ItemWidget->Initialize_Native(LayerData->MainCategoryCache, NAME_None, *ClassHash, Icon, BuildingName);
            MainCategoryItem->CategoryWidget->AddItem(ItemWidget, false);
            MainCategoryItem->Items.Add(FName{ FString::FromInt(*ClassHash) }, FMenuItem{ BuildingName.ToString(), ItemWidget });
        }
        else
        {
            FSubCategoryItem* SubCategoryItem = MainCategoryItem->SubCategories.Find(LayerData->SubCategoryCache);
            if (!SubCategoryItem)
            {
                UE_LOG(LogCartograph, Warning, TEXT("Can't find sub category %s"), *LayerData->SubCategoryCache.ToString());
                continue;
            }

            auto* ItemWidget = CreateWidget<UCartographMenuLayerItemWidget>(this, CategoryLayerItemWidgetType);
            ItemWidget->Initialize_Native(LayerData->MainCategoryCache, LayerData->SubCategoryCache, *ClassHash, Icon, BuildingName);
            SubCategoryItem->CategoryWidget->AddItem(ItemWidget, false);
            SubCategoryItem->Items.Add(FName{ FString::FromInt(*ClassHash) }, FMenuItem{ BuildingName.ToString(), ItemWidget });
        }
    }

    CARTO_LOG_DEBUG("Initialized layers");
}


void UCartographMenuWidget::PostInitialize()
{
    for (const auto& [_, HeadingItem] : MenuItemHierarchy)
    {
        HeadingItem.CategoryWidget->PostInitialize();
        for (const auto& [__, MainCategoryItem] : HeadingItem.MainCategories)
        {
            MainCategoryItem.CategoryWidget->PostInitialize();
            for (const auto& [___, SubCategoryItem] : MainCategoryItem.SubCategories)
            {
                SubCategoryItem.CategoryWidget->PostInitialize();
            }
        }
    }
}


void UCartographMenuWidget::UpdateVisibilities(FText SearchText)
{
    CARTO_LOG_DEBUG("UpdateVisibilities %s", *SearchText.ToString());

    const bool IsSearchTextEmpty = SearchText.IsEmptyOrWhitespace();
    const FString SearchString = SearchText.ToString();

    for (const auto& [_, HeadingItem] : MenuItemHierarchy)
    {
        const bool DoesHeadingMatch = !IsSearchTextEmpty && HeadingItem.DisplayName.Contains(SearchString);
        bool ShouldHeadingVisible = false;
        for (const auto& [__, MainCategoryItem] : HeadingItem.MainCategories)
        {
            const bool DoesMainCategoryMatch = !IsSearchTextEmpty && MainCategoryItem.DisplayName.Contains(SearchString);
            bool ShouldMainCategoryVisible = false;
            for (const auto& [___, SubCategoryItem] : MainCategoryItem.SubCategories)
            {
                const bool DoesSubCategoryMatch = !IsSearchTextEmpty && SubCategoryItem.DisplayName.Contains(SearchString);
                bool ShouldSubCategoryVisible = false;
                for (const auto& [____, Item] : SubCategoryItem.Items)
                {
                    const bool ShouldBeVisible = 
                        (IsSearchTextEmpty || DoesHeadingMatch || DoesMainCategoryMatch || DoesSubCategoryMatch || Item.DisplayName.Contains(SearchString)) &&
                        Item.Widget->ShouldBeVisible();

                    CARTO_LOG_DEBUG("Item %s %s", *Item.DisplayName, ShouldBeVisible ? TEXT("visible") : TEXT("hidden"));
                    Item.Widget->SetVisibility(ShouldBeVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
                    ShouldSubCategoryVisible |= ShouldBeVisible;
                }

                SubCategoryItem.CategoryWidget->SetVisibility(ShouldSubCategoryVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
            	ShouldMainCategoryVisible |= ShouldSubCategoryVisible;
            }

            MainCategoryItem.CategoryWidget->SetVisibility(ShouldMainCategoryVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
            ShouldHeadingVisible |= ShouldMainCategoryVisible;
        }

        HeadingItem.CategoryWidget->SetVisibility(ShouldHeadingVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}
