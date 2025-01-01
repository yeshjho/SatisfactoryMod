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


private:
	virtual const FText& GetTextForSearching() override;
};
