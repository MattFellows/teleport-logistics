#include "Modules/ModuleManager.h"
#include "TeleportLogisticsLog.h"
#include "TeleportLogisticsMap.h"
#include "UObject/CoreRedirects.h"

DEFINE_LOG_CATEGORY(LogTeleportLogistics);

class FTeleportLogisticsModule : public FDefaultGameModuleImpl
{
  public:
    virtual void StartupModule() override
    {
        // Saves made with the original Teleport Logistics mod reference its classes, structs and
        // enums as /Script/TeleportLogistics.*. This continuation keeps every type name and only
        // renames the module, so one package redirect lets those saves load unchanged.
        const FCoreRedirect Redirect(ECoreRedirectFlags::Type_Package, TEXT("/Script/TeleportLogistics"),
                                     TEXT("/Script/TeleportLogisticsContinued"));
        FCoreRedirects::AddRedirectList(MakeArrayView(&Redirect, 1), TEXT("TeleportLogisticsContinued"));
        TeleportLogisticsMap::RegisterTypes();
    }
};
IMPLEMENT_MODULE(FTeleportLogisticsModule, TeleportLogisticsContinued)
