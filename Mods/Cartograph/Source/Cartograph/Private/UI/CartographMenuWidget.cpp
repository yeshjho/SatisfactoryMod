#include "UI/CartographMenuWidget.h"

#include "CartographGameInstanceModule.h"
#include "CartographMenuCategoryWidget.h"
#include "PanelWidget.h"


void UCartographMenuWidget::InitializeHeadings(UPanelWidget* Panel)
{
    Algo::SortBy(HeadingData, &FHeadingData::Priority);
    for (const auto& [Name, DisplayName, Priority] : HeadingData)
    {
        UCartographMenuCategoryWidget* CategoryWidget = CreateWidget<UCartographMenuCategoryWidget>(this, CategoryWidgetType);
        CategoryWidget->Initialize(ECategoryType::Heading, DisplayName);
        Panel->AddChild(CategoryWidget);
        MenuItemHierarchy.Add(Name, FHeadingItem{ 
            DisplayName,
			{},
            CategoryWidget,
        });
    }
}


void UCartographMenuWidget::InitializeLayers(UPanelWidget* Panel)
{
    FHeadingItem& LayerHeading = *MenuItemHierarchy.Find("Layers");

    Algo::SortBy(GameInstanceModule->LayerCategories, &FLayerCategoryData::Priority);
    for (FLayerCategoryData& CategoryData : GameInstanceModule->LayerCategories)
    {
        Algo::SortBy(CategoryData.SubCategories, &FLayerSubCategoryData::Priority);

        UCartographMenuCategoryWidget* CategoryWidget = CreateWidget<UCartographMenuCategoryWidget>(this, CategoryWidgetType);
        CategoryWidget->Initialize(ECategoryType::MainCategory, CategoryData.DisplayName);
        LayerHeading.CategoryWidget->AddCategory(CategoryWidget);
        FMainCategoryItem MainCategoryItem{
            CategoryData.DisplayName,
			{},
            CategoryWidget,
        };

        for (FLayerSubCategoryData& SubCategoryData : CategoryData.SubCategories)
        {
            UCartographMenuCategoryWidget* SubCategoryWidget = CreateWidget<UCartographMenuCategoryWidget>(this, CategoryWidgetType);
            SubCategoryWidget->Initialize(ECategoryType::SubCategory, SubCategoryData.DisplayName);
            CategoryWidget->AddCategory(SubCategoryWidget);
            MainCategoryItem.SubCategories.Add(SubCategoryData.Name, FSubCategoryItem{
                .DisplayName = SubCategoryData.DisplayName,
                .CategoryWidget = SubCategoryWidget,
            });
        }

        LayerHeading.MainCategories.Add(CategoryData.Name, std::move(MainCategoryItem));
    }
}
