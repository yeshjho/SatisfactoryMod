#include "UI/CartographMenuLayerItemWidget.h"

#include "CartographGameInstanceModule.h"


void UCartographMenuLayerItemWidget::Initialize_Native(const FName& InLayerCategory, const FName& InLayerSubCategory,
                                                       uint32 InClassHash, UTexture2D* Icon, const FText& InBuildingName)
{
    LayerCategory = InLayerCategory;
    LayerSubCategory = InLayerSubCategory;
    ClassHash = InClassHash;
    BuildingName = InBuildingName;

    Initialize(Icon);
}


bool UCartographMenuLayerItemWidget::ShouldBeVisible() const
{
    return GameInstanceModule->DoesBuildingExist(ClassHash);
}


void UCartographMenuLayerItemWidget::OnClicked(bool IsChecked) const
{
    if (!IsChecked)
    {
        GameInstanceModule->RuntimeConfig.DisabledLayerBuildable.Add(ClassHash);
    }
    else
    {
        GameInstanceModule->RuntimeConfig.DisabledLayerBuildable.Remove(ClassHash);
    }
    GameInstanceModule->OnLayerConfigChanged();
}


bool UCartographMenuLayerItemWidget::GetInitialStatus() const
{
    return !GameInstanceModule->RuntimeConfig.DisabledLayerBuildable.Contains(ClassHash);
}
