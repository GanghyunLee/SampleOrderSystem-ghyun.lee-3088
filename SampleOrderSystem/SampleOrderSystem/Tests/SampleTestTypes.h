// Sample management tests exercise the production Sample / SampleRepository
// types directly. This header used to declare a standalone signature-only
// contract (see git history) so the tests could compile before the
// implementer agent wrote the real Model/ headers; now that
// SampleOrderSystem/SampleOrderSystem/Model/Sample.h and
// Model/SampleRepository.h exist, this file simply forwards to them so
// SampleRepositoryTest.cpp (which is never modified) keeps compiling
// unchanged.
#pragma once

#include "../Model/Sample.h"
#include "../Model/SampleRepository.h"
