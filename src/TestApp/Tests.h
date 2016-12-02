#pragma once

#include "../BlackBone/Config.h"
#include "../BlackBone/Process/Process.h"
#include "../BlackBone/PE/PEImage.h"
#include "../BlackBone/Misc/Utils.h"
#include "../BlackBone/Misc/DynImport.h"

#include <iostream>

using namespace blackbone;

void TestPatterns();
void TestLocalHook();
void TestRemoteHook();
void TestMMap();
void TestMMapFromMem();
void TestRemoteCall();
void TestDriver();
void TestRemoteMem();
void TestMultiPtr();