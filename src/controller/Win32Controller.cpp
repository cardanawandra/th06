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

#include <windows.h>

u16 Controller::GetInput()
{
    u16 buttons = 0;

    if (GetKeyState(VK_UP) & 0x8000)
        buttons |= TH_BUTTON_UP;

    if (GetKeyState(VK_DOWN) & 0x8000)
        buttons |= TH_BUTTON_DOWN;

    if (GetKeyState(VK_LEFT) & 0x8000)
        buttons |= TH_BUTTON_LEFT;

    if (GetKeyState(VK_RIGHT) & 0x8000)
        buttons |= TH_BUTTON_RIGHT;

    if (GetKeyState('Z') & 0x8000)
        buttons |= TH_BUTTON_SHOOT;

    if (GetKeyState('X') & 0x8000)
        buttons |= TH_BUTTON_BOMB;

    if (GetKeyState(VK_SHIFT) & 0x8000)
        buttons |= TH_BUTTON_FOCUS;

    if (GetKeyState(VK_ESCAPE) & 0x8000)
        buttons |= TH_BUTTON_MENU;

    if (GetKeyState(VK_CONTROL) & 0x8000)
        buttons |= TH_BUTTON_SKIP;

    if (GetKeyState('Q') & 0x8000)
        buttons |= TH_BUTTON_Q;

    if (GetKeyState('S') & 0x8000)
        buttons |= TH_BUTTON_S;

    if (GetKeyState(VK_RETURN) & 0x8000)
        buttons |= TH_BUTTON_ENTER;

    return buttons;
}

void Controller::ResetKeyboard(void)
{
}
