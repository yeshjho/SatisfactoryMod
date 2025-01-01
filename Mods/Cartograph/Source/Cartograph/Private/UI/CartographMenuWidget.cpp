#include "UI/CartographMenuWidget.h"

#include "PanelWidget.h"

#include "FGBuildable.h"
#include "FGBuildingDescriptor.h"
#include "FGBuildSubCategory.h"
#include "FGRecipeManager.h"

#include "CartographGameInstanceModule.h"
#include "CartographLayerToggleItemWidget.h"
#include "CartographMenuCategoryWidget.h"
#include "CartographMenuLayerItemWidget.h"


void UCartographMenuWidget::InitializeHeadings(UPanelWidget* Panel)
{
    Algo::SortBy(HeadingData, &FHeadingData::Priority);
    for (const auto& [Name, DisplayName, Priority] : HeadingData)
    {
        auto* CategoryWidget = CreateWidget<UCartographMenuCategoryWidget>(this, CategoryWidgetType);
        CategoryWidget->Initialize(ECategoryType::Heading, DisplayName);
        Panel->AddChild(CategoryWidget);
        MenuItemHierarchy.Add(Name, FHeadingItem{ 
            DisplayName,
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
        ToggleItemWidget->Initialize(CategoryData.Name, FName{});
        CategoryWidget->AddItem(ToggleItemWidget, true);
        FMainCategoryItem MainCategoryItem{
            CategoryData.DisplayName,
			{},
            CategoryWidget,
        };

        for (FLayerSubCategoryData& SubCategoryData : CategoryData.SubCategories)
        {
            auto* SubCategoryWidget = CreateWidget<UCartographMenuCategoryWidget>(this, CategoryWidgetType);
            SubCategoryWidget->Initialize(ECategoryType::SubCategory, SubCategoryData.DisplayName);
            CategoryWidget->AddCategory(SubCategoryWidget);
            auto* ToggleItemWidgetSub= CreateWidget<UCartographLayerToggleItemWidget>(this, CategoryLayerToggleItemWidgetType);
            ToggleItemWidget->Initialize(CategoryData.Name, SubCategoryData.Name);
            SubCategoryWidget->AddItem(ToggleItemWidgetSub, true);
            MainCategoryItem.SubCategories.Add(SubCategoryData.Name, FSubCategoryItem{
                .DisplayName = SubCategoryData.DisplayName,
                .CategoryWidget = SubCategoryWidget,
            });
        }

        LayerHeading.MainCategories.Add(CategoryData.Name, std::move(MainCategoryItem));
    }

    const AFGRecipeManager* RecipeManager = AFGRecipeManager::Get(UCartographGameInstanceModule::Instance->GetWorld());
    for (const auto& [OriginalBuildableClass, _] : UCartographGameInstanceModule::Instance->ClassPtrToClassIDMap)
    {
	    if (!OriginalBuildableClass)
	    {
            continue;
	    }

        const auto& BuildableClassRedirectMap = UCartographGameInstanceModule::Instance->BuildableClassRedirectMap;
        const TSoftClassPtr<AFGBuildable>* RedirectClass = BuildableClassRedirectMap.Find(OriginalBuildableClass.Get());
        const TSubclassOf<AFGBuildable> BuildableClass = RedirectClass ? RedirectClass->LoadSynchronous() : OriginalBuildableClass.Get();

        const TSubclassOf<UFGBuildingDescriptor> Descriptor = RecipeManager->FindBuildingDescriptorByClass(BuildableClass);
        if (!Descriptor)
        {
            UE_LOG(LogCartograph, Warning, TEXT("Can't find descriptor for %s"), *BuildableClass->GetName());
            continue;
        }
        const FText BuildingName = UFGItemDescriptor::GetItemName(Descriptor);
        UTexture2D* Icon = UFGItemDescriptor::GetSmallIcon(Descriptor);

        auto& BuildableBuildLayerDataOverrideMap = UCartographGameInstanceModule::Instance->BuildableBuildLayerDataOverrideMap;
        const FBuildLayerData* LayerData = BuildableBuildLayerDataOverrideMap.Find(BuildableClass.Get());
        if (!LayerData)
        {
            auto& BuildLayerDataMap = UCartographGameInstanceModule::Instance->BuildLayerDataMap;
            if (TArray<TSubclassOf<UFGCategory>> Subcategories = UFGItemDescriptor::GetSubCategoriesOfClass(Descriptor, UFGBuildSubCategory::StaticClass());
                !Subcategories.IsEmpty())
            {
                LayerData = BuildLayerDataMap.Find(Subcategories[0].Get());
            }
            if (!LayerData)
            {
                const TSubclassOf<UFGBuildCategory> Category = UFGBuildingDescriptor::GetBuildCategory(Descriptor);
                LayerData = BuildLayerDataMap.Find(Category.Get());
            }
        }

        if (!LayerData)
        {
            UE_LOG(LogCartograph, Warning, TEXT("Can't find layer data for %s"), *BuildableClass->GetName());
            continue;
        }

        TArray<FString> CategoryNames;
        LayerData->Category.ParseIntoArray(CategoryNames, TEXT("/"));

        const FName MainCategoryName{ CategoryNames[0] };
        FMainCategoryItem* MainCategoryItem = LayerHeading.MainCategories.Find(MainCategoryName);
        if (!MainCategoryItem)
        {
            UE_LOG(LogCartograph, Warning, TEXT("Can't find main category %s"), *MainCategoryName.ToString());
            continue;
        }

        if (CategoryNames.Num() == 1)
        {
            auto* ItemWidget = CreateWidget<UCartographMenuLayerItemWidget>(this, CategoryLayerItemWidgetType);
            ItemWidget->Initialize(MainCategoryName, NAME_None, Icon, BuildingName);
            MainCategoryItem->CategoryWidget->AddItem(ItemWidget, false);
            MainCategoryItem->Items.Add(NAME_None, FMenuItem{ BuildingName , ItemWidget });
        }
        else
        {
            const FName SubCategoryName{ CategoryNames[1] };
            FSubCategoryItem* SubCategoryItem = MainCategoryItem->SubCategories.Find(SubCategoryName);
            if (!SubCategoryItem)
            {
                UE_LOG(LogCartograph, Warning, TEXT("Can't find sub category %s"), *SubCategoryName.ToString());
                continue;
            }

            auto* ItemWidget = CreateWidget<UCartographMenuLayerItemWidget>(this, CategoryLayerItemWidgetType);
            ItemWidget->Initialize(MainCategoryName, SubCategoryName, Icon, BuildingName);
            SubCategoryItem->CategoryWidget->AddItem(ItemWidget, false);
            SubCategoryItem->Items.Add(NAME_None, FMenuItem{ BuildingName , ItemWidget });
        }
    }
}


void UCartographMenuWidget::PostInitialize()
{
    for (const auto& [_, HeadingItem] : MenuItemHierarchy)
    {
        HeadingItem.CategoryWidget->PostInitialize();
        for (const auto& [_, MainCategoryItem] : HeadingItem.MainCategories)
        {
            MainCategoryItem.CategoryWidget->PostInitialize();
            for (const auto& [_, SubCategoryItem] : MainCategoryItem.SubCategories)
            {
                SubCategoryItem.CategoryWidget->PostInitialize();
            }
        }
    }
}
