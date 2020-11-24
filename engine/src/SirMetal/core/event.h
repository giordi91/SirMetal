#pragma once
#include <stdint.h>

namespace SirMetal{

enum class EVENT_TYPE {
  NONE = 0,
  WindowClose,
  WindowResize,
  WindowMoved,
  KeyPressed,
  KeyReleased,
  KeyTyped,
  MouseButtonPressed,
  MouseButtonReleased,
  MouseMoved,
  MouseScrolled,
};

constexpr uint32_t setBit(const uint32_t bit) { return 1 << bit; }

enum EVENT_CATEGORY {
  NONE = 0,
  EventCategoryApplication = setBit(0),
  EventCategoryInput = setBit(1),
  EventCategoryKeyboard = setBit(2),
  EventCategoryMouse = setBit(3),
  EventCategoryMouseButton = setBit(4),
  EventCategoryDebug = setBit(5),
  EventCategoryRendering = setBit(6),
  EventCategoryShaderCompile = setBit(7)
};

struct Event {
  inline bool isInCategory(const EVENT_CATEGORY category) const {
    return m_category & category;
  }

  EVENT_TYPE m_type{};
  EVENT_CATEGORY m_category{};
  // TODO here we are going to make a union of structs for the different events
  // we are also going to have a series of helper functions to build specific
  // events
};

}  // namespace BlackHole
