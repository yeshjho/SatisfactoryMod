#include "UI/CartographMenuLayerItemWidget.h"

#include "CartographGameInstanceModule.h"


void UCartographMenuLayerItemWidget::Initialize_Native(const FName& LayerCategory, const FName& LayerSubCategory,
                                                       uint32 ClassHash, UTexture2D* Icon, const FText& BuildingName)
{
    this->LayerCategory = LayerCategory;
    this->LayerSubCategory = LayerSubCategory;
    this->ClassHash = ClassHash;
    this->BuildingName = BuildingName;

    Initialize(Icon);
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
