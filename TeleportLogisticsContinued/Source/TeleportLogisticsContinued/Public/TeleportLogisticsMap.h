#pragma once
#include "CoreMinimal.h"
#include "FGActorRepresentation.h"
#include "TeleportLogisticsMap.generated.h"
namespace TeleportLogisticsMap {
    TELEPORTLOGISTICSCONTINUED_API ERepresentationType Logistics();
    TELEPORTLOGISTICSCONTINUED_API ERepresentationType Personnel();
    void RegisterTypes();
}
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsMapHooks : public UObject {
    GENERATED_BODY()
public:
    UFUNCTION() static FText CategoryName(ERepresentationType mRepresentationType, FText OriginalValue);
    static void Register(class UGameInstance* Instance);
};
