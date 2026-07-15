// Order management tests exercise the production Order / OrderStatus /
// OrderRepository types directly. This header used to declare a standalone
// signature-only contract so the tests could compile before the
// implementer agent wrote the real Model/ headers; now that
// SampleOrderSystem/SampleOrderSystem/Model/Order.h,
// Model/OrderStatus.h and Model/OrderRepository.h exist, this file simply
// forwards to them so OrderRepositoryTest.cpp (which is never modified)
// keeps compiling unchanged.
#pragma once

#include "../Model/Order.h"
#include "../Model/OrderStatus.h"
#include "../Model/OrderRepository.h"
