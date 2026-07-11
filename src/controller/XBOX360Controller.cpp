#include "../Controller.hpp"

#include "../GameErrorContext.hpp"
#include "../Supervisor.hpp"
#include "../i18n.hpp"
#include "../utils.hpp"
#include <xtl.h>
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

// SUPER SIMPLIFIED
u16 Controller::GetInput()
{
    u16 buttons = 0;

    XINPUT_STATE state = {};
    if (XInputGetState(0, &state) == ERROR_SUCCESS)
    {
        WORD b = state.Gamepad.wButtons;

        if (b & XINPUT_GAMEPAD_DPAD_UP)    buttons |= TH_BUTTON_UP;
        if (b & XINPUT_GAMEPAD_DPAD_DOWN)  buttons |= TH_BUTTON_DOWN;
        if (b & XINPUT_GAMEPAD_DPAD_LEFT)  buttons |= TH_BUTTON_LEFT;
        if (b & XINPUT_GAMEPAD_DPAD_RIGHT) buttons |= TH_BUTTON_RIGHT;

        if (b & XINPUT_GAMEPAD_A)          buttons |= TH_BUTTON_SHOOT;
        if (b & XINPUT_GAMEPAD_B)          buttons |= TH_BUTTON_BOMB;
        if (b & XINPUT_GAMEPAD_X)          buttons |= TH_BUTTON_FOCUS;
        if (b & XINPUT_GAMEPAD_START)      buttons |= TH_BUTTON_MENU;
        if (b & XINPUT_GAMEPAD_BACK)       buttons |= TH_BUTTON_SKIP;

        // Optional: map left stick with a deadzone.
    }

    return buttons;
}
void Controller::ResetKeyboard(void)
{
}
