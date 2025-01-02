#pragma once

#include "CoreMinimal.h"
#include "UI/CartographMenuItemWidget.h"
#include "CartographMenuLayerItemWidget.generated.h"


class UCartographGameInstanceModule;


/**
 * 
 */
UCLASS()
class CARTOGRAPH_API UCartographMenuLayerItemWidget : public UCartographMenuItemWidget
{
	GENERATED_BODY()

public:
    void Initialize_Native(const FName& LayerCategory, const FName& LayerSubCategory, uint32 ClassHash, UTexture2D* Icon, const FText& BuildingName);

    UFUNCTION(BlueprintImplementableEvent)
    void Initialize(UTexture2D* Icon);

    virtual bool ShouldBeVisible() const override;

    uint32 GetClassHash() const { return ClassHash; }


private:
    UFUNCTION(BlueprintCallable)
    void OnClicked(bool IsChecked) const;

    UFUNCTION(BlueprintCallable)
    bool GetInitialStatus() const;


protected:
    UPROPERTY(BlueprintReadWrite)
    UCartographGameInstanceModule* GameInstanceModule;


    UPROPERTY(BlueprintReadWrite)
    FName LayerCategory;

    UPROPERTY(BlueprintReadWrite)
    FName LayerSubCategory;

    UPROPERTY(BlueprintReadWrite)
    FText BuildingName;

    uint32 ClassHash;
};
