#pragma once
#include <string>

//#include "blackHole/core/event.h"

namespace SirMetal{
struct EngineContext;
struct Event;

// This is the Layer base class, the game can subclass this and implement
// whatever custom logic the game needs to perform
class Layer {
 public:
  explicit Layer(std::string name) : m_debugName(std::move(name)) {}
  explicit Layer(const char* debugName) : m_debugName(debugName) {}
  virtual ~Layer() = default;

  virtual void onAttach(EngineContext* context) = 0;
  virtual void onDetach() = 0;
  virtual void onUpdate() = 0;
  virtual bool onEvent(Event& event) = 0;
  virtual void clear() = 0;

  inline const std::string& getName() const { return m_debugName; }

 protected:
  std::string m_debugName;
};

}  // namespace BlackHole
