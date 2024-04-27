// Copyright ©xukai. All Rights Reserved.

#include "XkGamedevCore.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FXkGamedevCoreModule"

void FXkGamedevCoreModule::StartupModule()
{
	FString ShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("XkGamedevKit"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/XkGamedevCore"), ShaderDir);
}

void FXkGamedevCoreModule::ShutdownModule()
{
}

IMPLEMENT_GAME_MODULE(FXkGamedevCoreModule, XkGamedevCore);

#undef LOCTEXT_NAMESPACE
 