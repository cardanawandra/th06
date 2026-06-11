#define CONTROLLER_INIT_COMPAT()
#define START_TEXT_INPUT_COMPAT()
#define STOP_TEXT_INPUT_COMPAT()

#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Keyboard.hpp>

#undef KEYBOARD_KEY_PRESSED
#define KEYBOARD_KEY_PRESSED(button, key) \
    (sf::Keyboard::isKeyPressed(key) ? (button) : 0)
#define GET_KEYSTATE_COMPAT() 0

#define JOYSTICK_COMPAT unsigned int

#define JOYSTICK_COMPATOpen(id) 0
#define JOYSTICK_COMPATClose(id)

#define JOYSTICK_COMPATGetJoystick(a) NULL

#define CONTROLLER_AXIS_LEFTX  sf::Joystick::Axis::X
#define CONTROLLER_AXIS_LEFTY  sf::Joystick::Axis::Y
#define CONTROLLER_AXIS_RIGHTX sf::Joystick::Axis::U
#define CONTROLLER_AXIS_RIGHTY sf::Joystick::Axis::R

#define JOYSTICK_COMPATHasAxis(id, axis) \
    sf::Joystick::hasAxis(*id, axis)

#define JOYSTICK_COMPATGetAxis(id, axis) \
    sf::Joystick::getAxisPosition(*id, axis)

#define JOYSTICK_COMPATGetButton(id, button) \
    sf::Joystick::isButtonPressed(*id, button)

#define KEY_UP             sf::Keyboard::Key::Up
#define KEY_DOWN           sf::Keyboard::Key::Down
#define KEY_LEFT           sf::Keyboard::Key::Left
#define KEY_RIGHT          sf::Keyboard::Key::Right

#define KEY_KP8            sf::Keyboard::Key::Numpad8
#define KEY_KP2            sf::Keyboard::Key::Numpad2
#define KEY_KP4            sf::Keyboard::Key::Numpad4
#define KEY_KP6            sf::Keyboard::Key::Numpad6

#define KEY_KP7            sf::Keyboard::Key::Numpad7
#define KEY_KP9            sf::Keyboard::Key::Numpad9
#define KEY_KP1            sf::Keyboard::Key::Numpad1
#define KEY_KP3            sf::Keyboard::Key::Numpad3

#define KEY_HOME           sf::Keyboard::Key::Home

#define KEY_Z              sf::Keyboard::Key::Z
#define KEY_X              sf::Keyboard::Key::X

#define KEY_LSHIFT         sf::Keyboard::Key::LShift
#define KEY_RSHIFT         sf::Keyboard::Key::RShift

#define KEY_ESCAPE         sf::Keyboard::Key::Escape

#define KEY_LCTRL          sf::Keyboard::Key::LControl
#define KEY_RCTRL          sf::Keyboard::Key::RControl

#define KEY_Q              sf::Keyboard::Key::Q
#define KEY_S              sf::Keyboard::Key::S

#define KEY_RETURN         sf::Keyboard::Key::Enter
#define PUMP_EVENTS_COMPAT()