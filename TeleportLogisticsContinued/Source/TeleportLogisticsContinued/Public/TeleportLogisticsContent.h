#pragma once
#include "CoreMinimal.h"
#include "FGBuildCategory.h"
#include "FGBuildSubCategory.h"
#include "Resources/FGBuildingDescriptor.h"
#include "FGRecipe.h"
#include "FGSchematic.h"
#include "Unlocks/FGUnlockRecipe.h"
#include "Module/GameInstanceModule.h"
#include "Module/GameWorldModule.h"
#include "TeleportLogisticsContent.generated.h"

UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsCategory : public UFGBuildCategory
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsCategory();
};
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsSubCategory : public UFGBuildSubCategory
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsSubCategory();
};
UCLASS(Abstract)
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsDescriptor : public UFGBuildingDescriptor
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsDescriptor();

  protected:
    void Icon(const TCHAR *SmallPath, const TCHAR *BigPath);
};
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsItemInputDescriptor : public UTeleportLogisticsDescriptor
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsItemInputDescriptor();
};
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsItemOutputDescriptor : public UTeleportLogisticsDescriptor
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsItemOutputDescriptor();
};
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsFluidInputDescriptor : public UTeleportLogisticsDescriptor
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsFluidInputDescriptor();
};
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsFluidOutputDescriptor : public UTeleportLogisticsDescriptor
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsFluidOutputDescriptor();
};
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsHubDescriptor : public UTeleportLogisticsDescriptor
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsHubDescriptor();
};
UCLASS(Abstract)
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsRecipe : public UFGRecipe
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsRecipe();
};
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsItemInputRecipe : public UTeleportLogisticsRecipe
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsItemInputRecipe();
};
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsItemOutputRecipe : public UTeleportLogisticsRecipe
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsItemOutputRecipe();
};
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsFluidInputRecipe : public UTeleportLogisticsRecipe
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsFluidInputRecipe();
};
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsFluidOutputRecipe : public UTeleportLogisticsRecipe
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsFluidOutputRecipe();
};
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsHubRecipe : public UTeleportLogisticsRecipe
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsHubRecipe();
};
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsUnlock : public UFGUnlockRecipe
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsUnlock();
};
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsMilestone : public UFGSchematic
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsMilestone();
};
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsGameInstanceModule : public UGameInstanceModule
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsGameInstanceModule();
    UFUNCTION(BlueprintCallable) static void PrepareRewardPresentation();
    virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
};
UCLASS()
class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsWorldModule : public UGameWorldModule
{
    GENERATED_BODY()
  public:
    UTeleportLogisticsWorldModule();
};

UCLASS() class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsTravelDescriptor : public UTeleportLogisticsDescriptor { GENERATED_BODY() public: UTeleportLogisticsTravelDescriptor(); };
UCLASS() class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsTravelRecipe : public UTeleportLogisticsRecipe { GENERATED_BODY() public: UTeleportLogisticsTravelRecipe(); };
UCLASS() class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsTravelUnlock : public UFGUnlockRecipe { GENERATED_BODY() public: UTeleportLogisticsTravelUnlock(); };
UCLASS() class TELEPORTLOGISTICSCONTINUED_API UTeleportLogisticsTravelMilestone : public UFGSchematic { GENERATED_BODY() public: UTeleportLogisticsTravelMilestone(); };
