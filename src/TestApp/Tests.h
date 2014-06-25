#pragma once

#include "../BlackBone/Process/Process.h"
#include "../BlackBone/PE/PEParser.h"
#include "../BlackBone/Misc/Utils.h"
#include "../BlackBone/Misc/DynImport.h"

#include <iostream>

using namespace blackbone;

void TestLocalHook();
void TestRemoteHook();
void TestMMap();
void TestRemoteCall();
void TestDriver();
void TestRemoteMem();