#pragma once

#include "CoreMinimal.h"
#include "UI/CartographMenuItemWidget.h"
#include "CartographMenuLayerItemWidget.generated.h"


/**
 * 
 */
UCLASS()
class CARTOGRAPH_API UCartographMenuLayerItemWidget : public UCartographMenuItemWidget
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent)
    void Initialize(const FName& LayerCategory, const FName& LayerSubCategory, UTexture2D* Icon, const FText& BuildingName);


private:


protected:
    UPROPERTY(BlueprintReadWrite)
    FName LayerCategory;

    UPROPERTY(BlueprintReadWrite)
    FName LayerSubCategory;

    UPROPERTY(BlueprintReadWrite)
    FText BuildingName;
};
