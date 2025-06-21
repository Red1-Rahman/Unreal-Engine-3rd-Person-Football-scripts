#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FFootballGameModule : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
