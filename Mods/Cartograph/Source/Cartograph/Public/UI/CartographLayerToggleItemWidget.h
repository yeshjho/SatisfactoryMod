#pragma once

#include "CoreMinimal.h"
#include "UI/CartographMenuItemWidget.h"
#include "CartographLayerToggleItemWidget.generated.h"


class UCartographGameInstanceModule;
class UCartographMenuWidget;


/**
 * 
 */
UCLASS()
class CARTOGRAPH_API UCartographLayerToggleItemWidget : public UCartographMenuItemWidget
{
	GENERATED_BODY()

public:
    void Initialize_Native(UCartographMenuWidget* InMenuWidget, const FName& InMainCategory, const FName& InSubCategory);

private:
    UFUNCTION(BlueprintCallable)
    void OnClicked(bool IsChecked) const;

    UFUNCTION(BlueprintCallable)
    bool GetInitialStatus() const;

    UFUNCTION(BlueprintCallable)
    void SelectOrDeselectAll(bool DoSelect);


protected:
    UPROPERTY(BlueprintReadWrite)
    UCartographGameInstanceModule* GameInstanceModule;


protected:
    UPROPERTY(BlueprintReadWrite)
    FName MainCategory;

    UPROPERTY(BlueprintReadWrite)
    FName SubCategory;

    UPROPERTY(BlueprintReadWrite)
    UCartographMenuWidget* MenuWidget;
};
