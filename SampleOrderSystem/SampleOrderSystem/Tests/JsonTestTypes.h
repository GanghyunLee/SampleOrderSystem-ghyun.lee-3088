// Stage-0 foundation tests exercise the production JsonValue / JsonParser /
// SaveManager types directly. This header used to declare a standalone
// signature-only contract (see git history) so the tests could compile
// before the implementer agent wrote the real Model/ headers; now that
// SampleOrderSystem/SampleOrderSystem/Model/JsonValue.h, JsonParser.h and
// SaveManager.h exist, this file simply forwards to them so the *Test.cpp
// files (which are never modified) keep compiling unchanged.
#pragma once

#include "../Model/JsonValue.h"
#include "../Model/JsonParser.h"
#include "../Model/SaveManager.h"
