#pragma once

// Order lifecycle states. See docs/PRD.md section 5 and
// docs/FEATURES/03-order-registration.md / 04-order-approval-rejection.md
// for the transitions between these states.
enum class OrderStatus { RESERVED, REJECTED, PRODUCING, CONFIRMED, RELEASE };
