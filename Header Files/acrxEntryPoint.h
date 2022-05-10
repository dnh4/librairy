#pragma once

#include "cmdFmap.h"

void initApp();
void initCmd( AcString sCmdName, GcRxFunctionPtr FunctionAddr );
void unloadApp();
extern "C" AcRx::AppRetCode acrxEntryPoint( AcRx::AppMsgCode msg, void* appId );


//Declarer le groupe name
AcString sGroupName = "TEMPLATE";