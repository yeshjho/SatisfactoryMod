#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CartographMenuWidget.generated.h"


class UCartographGameInstanceModule;
class UCartographMenuCategoryWidget;
class UCartographMenuItemWidget;


USTRUCT(BlueprintType)
struct FHeadingData
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FName Name;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    int Priority;
};


USTRUCT(BlueprintType)
struct FMenuItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString DisplayName;

    UPROPERTY(BlueprintReadWrite)
    UCartographMenuItemWidget* Widget;
};


USTRUCT(BlueprintType)
struct FSubCategoryItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString DisplayName;

    UPROPERTY(BlueprintReadWrite)
    TMap<FName, FMenuItem> Items;

    UPROPERTY(BlueprintReadWrite)
    UCartographMenuCategoryWidget* CategoryWidget;
};


USTRUCT(BlueprintType)
struct FMainCategoryItem : public FSubCategoryItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TMap<FName, FSubCategoryItem> SubCategories;
};


USTRUCT(BlueprintType)
struct FHeadingItem : public FSubCategoryItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TMap<FName, FMainCategoryItem> MainCategories;
};


/**
 * 
 */
UCLASS()
class CARTOGRAPH_API UCartographMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    const TMap<FName, FHeadingItem>& GetMenuItemHierarchy() const { return MenuItemHierarchy; }


private:
    UFUNCTION(BlueprintCallable)
    void InitializeHeadings(UPanelWidget* Panel);

	UFUNCTION(BlueprintCallable)
	void InitializeLayers();

    UFUNCTION(BlueprintCallable)
    void PostInitialize();

    UFUNCTION(BlueprintCallable)
    void UpdateVisibilities(FText SearchText);


protected:
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UUserWidget> CategoryWidgetType;

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UUserWidget> CategoryLayerToggleItemWidgetType;

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UUserWidget> CategoryLayerItemWidgetType;

    UPROPERTY(EditDefaultsOnly)
    TArray<FHeadingData> HeadingData;


    UPROPERTY(BlueprintReadWrite)
    UCartographGameInstanceModule* GameInstanceModule;

    UPROPERTY(BlueprintReadWrite)
    TMap<FName, FHeadingItem> MenuItemHierarchy;
};
