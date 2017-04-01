#pragma once
#include <Catch/catch.hpp>
#include "../BlackBone/Config.h"
#include "../BlackBone/Process/Process.h"
#include "../BlackBone/PE/PEImage.h"
#include "../BlackBone/Misc/Utils.h"
#include "../BlackBone/Misc/DynImport.h"

#include <iostream>

using namespace blackbone;

#define CHECK_NT_SUCCESS(Status)    CHECK((NTSTATUS)(Status) >= 0)
#define REQUIRE_NT_SUCCESS(Status)  REQUIRE((NTSTATUS)(Status) >= 0)

void TestMMap();
void TestMMapFromMem();