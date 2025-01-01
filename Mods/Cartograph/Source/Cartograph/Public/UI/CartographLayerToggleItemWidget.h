#pragma once

#include "CoreMinimal.h"
#include "UI/CartographMenuItemWidget.h"
#include "CartographLayerToggleItemWidget.generated.h"


/**
 * 
 */
UCLASS()
class CARTOGRAPH_API UCartographLayerToggleItemWidget : public UCartographMenuItemWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void Initialize(const FName& MainCategory, const FName& SubCategory);

private:


protected:
    UPROPERTY(BlueprintReadWrite)
    FName MainCategory;

    UPROPERTY(BlueprintReadWrite)
    FName SubCategory;
};
