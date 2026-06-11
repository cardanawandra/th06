#include "Controller.hpp"

#include "GameErrorContext.hpp"
#include "Supervisor.hpp"
#include "i18n.hpp"
#include "utils.hpp"

// DIFFABLE_STATIC(JOYCAPSA, g_JoystickCaps)
static u16 g_FocusButtonConflictState;
static u8 *keyboardState;

u16 Controller::GetJoystickCaps(void)
{
    return 0;
}

u16 Controller::GetControllerInput(u16 buttons)
{
    return buttons;
}

u32 Controller::SetButtonFromDirectInputJoystate(u16 *outButtons, i16 controllerButtonToTest,
                                                 enum TouhouButton touhouButton, const u8 *inputButtons)
{
    return 0;
}

u32 Controller::SetButtonFromControllerInputs(u16 *outButtons, i16 controllerButtonToTest,
                                              enum TouhouButton touhouButton, JOYSTICK_COMPAT *controller)
{
    return 0;
}

static u8 g_ControllerData[COMPAT_CONTROLLER_BUTTON_MAX];

// This is for rebinding keys
const u8 *Controller::GetControllerState()
{
    return g_ControllerData;
}

u16 Controller::GetInput(void)
{
    // //only works on sdl1.2
    // PUMP_EVENTS_COMPAT();
    // u16 buttons = 0;

    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP, KEY_UP);
    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN, KEY_DOWN);
    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_LEFT, KEY_LEFT);
    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_RIGHT, KEY_RIGHT);

    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP, KEY_KP8);
    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN, KEY_KP2);
    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_LEFT, KEY_KP4);
    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_RIGHT, KEY_KP6);

    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP_LEFT, KEY_KP7);
    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP_RIGHT, KEY_KP9);
    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN_LEFT, KEY_KP1);
    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN_RIGHT, KEY_KP3);

    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_HOME, KEY_HOME);

    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_SHOOT, KEY_Z);
    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_BOMB, KEY_X);

    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_FOCUS, KEY_LSHIFT);
    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_FOCUS, KEY_RSHIFT);

    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_MENU, KEY_ESCAPE);

    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_SKIP, KEY_LCTRL);
    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_SKIP, KEY_RCTRL);

    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_Q, KEY_Q);
    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_S, KEY_S);
    // buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_ENTER, KEY_RETURN);

    // return Controller::GetControllerInput(buttons);
    return 0;
}

void Controller::ResetKeyboard(void)
{
}
