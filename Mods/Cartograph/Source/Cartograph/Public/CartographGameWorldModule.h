#pragma once

#include "CoreMinimal.h"
#include "Module/GameWorldModule.h"
#include "CartographGameWorldModule.generated.h"


class UCartographGameInstanceModule;


/**
 * 
 */
UCLASS()
class CARTOGRAPH_API UCartographGameWorldModule : public UGameWorldModule
{
	GENERATED_BODY()

public:
	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
	virtual void BeginDestroy() override;
};
