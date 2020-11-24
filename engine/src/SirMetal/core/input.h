#pragma once

#include <SDL_scancode.h>
#include <memory.h>

constexpr uint8_t MOUSE_BUTTON_MAX = 4;


namespace SirMetal{
// NOTE: should we use something more general?
struct MousePosition {
  int x;
  int y;
  int xRel;
  int yRel;
};

struct MouseData {
  Uint8 buttons[MOUSE_BUTTON_MAX];
  Uint8 buttonsPrev[MOUSE_BUTTON_MAX];
  MousePosition position;
  MousePosition positionPrev;
};

class Input {
 public:

  MouseData mouse;
  uint8_t m_keys[SDL_NUM_SCANCODES] = {};
  uint8_t m_keysPrev[SDL_NUM_SCANCODES] = {};

  void initialize() {
    memset(m_keys, 0, sizeof(uint8_t) * SDL_NUM_SCANCODES);
    memset(m_keysPrev, 0, sizeof(uint8_t) * SDL_NUM_SCANCODES);
  }

  void cleanup(){};

  // Note: KEYBOARD INPUT HANDLING
  inline void keyDown(const SDL_Scancode input) { m_keys[input] = 1u; }
  inline void keyUp(const SDL_Scancode input) { m_keys[input] = 0u; }

  inline uint8_t isKeyDown(const SDL_Scancode input) const {
    return m_keys[input];
  }

  inline uint8_t wasKeyDown(const SDL_Scancode input) const {
    return m_keysPrev[input];
  }

  inline bool isKeyReleased(const SDL_Scancode input) const {
    return (m_keys[input] == 0) & (m_keysPrev[input] != 0);
  }

  inline bool isKeyPressedThisFrame(const SDL_Scancode input) const {
    return (m_keys[input] != 0) & (m_keysPrev[input] == 0);
  }

  // Note: MOUSE INPUT HANDLING
  inline void mouseButtonDown(const Uint8 input) { mouse.buttons[input] = 1u; }
  inline void mouseButtonUp(const Uint8 input) { mouse.buttons[input] = 0u; }

  inline uint8_t isMouseButtonDown(const Uint8 input) const {
    return mouse.buttons[input];
  }

  inline uint8_t wasMouseButtonDown(const Uint8 input) const {
    return mouse.buttonsPrev[input];
  }

  inline bool isMouseButtonReleased(const Uint8 input) const {
    return (mouse.buttons[input] == 0) & (mouse.buttonsPrev[input] != 0);
  }

  inline bool isMouseButtonPressedThisFrame(const int input) const {
    return (mouse.buttons[input] != 0) & (mouse.buttonsPrev[input] == 0);
  }

  void swapInputBuffers() {
    memcpy(m_keysPrev, m_keys, sizeof(uint8_t) * SDL_NUM_SCANCODES);
    memcpy(mouse.buttonsPrev, mouse.buttons, sizeof(Uint8) * MOUSE_BUTTON_MAX);
    memcpy(&mouse.positionPrev, &mouse.position, sizeof(MousePosition));

  }
};
}  // namespace BlackHole
