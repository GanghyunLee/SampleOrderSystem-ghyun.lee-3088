// Production queue tests exercise the production ProductionJob /
// ProductionQueue types directly. This header used to declare a standalone
// signature-only contract (see git history) so the tests could compile
// (but not correctly link/run) before the implementer agent wrote the real
// Model/ headers; now that SampleOrderSystem/SampleOrderSystem/Model/
// ProductionJob.h and Model/ProductionQueue.h exist, this file simply
// forwards to them so OrderApprovalTest.cpp (which is never modified) keeps
// compiling unchanged, matching the OrderTestTypes.h / SampleTestTypes.h
// precedent.
#pragma once

#include "../Model/ProductionJob.h"
#include "../Model/ProductionQueue.h"
