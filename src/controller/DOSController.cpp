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

#include <conio.h>

#define KEY_UP      72
#define KEY_DOWN    80
#define KEY_LEFT    75
#define KEY_RIGHT   77

#define KEY_ESC     27
#define KEY_ENTER   13

u16 Controller::GetInput(void)
{
    u16 buttons = 0;

    while (kbhit())
    {
        int key = getch();

        // extended keys (arrows, keypad)
        if (key == 0 || key == 224)
            key = getch();

        switch(key)
        {
            case KEY_UP:
                buttons |= TH_BUTTON_UP;
                break;

            case KEY_DOWN:
                buttons |= TH_BUTTON_DOWN;
                break;

            case KEY_LEFT:
                buttons |= TH_BUTTON_LEFT;
                break;

            case KEY_RIGHT:
                buttons |= TH_BUTTON_RIGHT;
                break;


            case 'z':
            case 'Z':
                buttons |= TH_BUTTON_SHOOT;
                break;

            case 'x':
            case 'X':
                buttons |= TH_BUTTON_BOMB;
                break;


            case 0x2A: // left shift
            case 0x36: // right shift
                buttons |= TH_BUTTON_FOCUS;
                break;


            case 27:
                buttons |= TH_BUTTON_MENU;
                break;


            case 0x1D: // ctrl
                buttons |= TH_BUTTON_SKIP;
                break;


            case 'q':
            case 'Q':
                buttons |= TH_BUTTON_Q;
                break;


            case 's':
            case 'S':
                buttons |= TH_BUTTON_S;
                break;


            case KEY_ENTER:
                buttons |= TH_BUTTON_ENTER;
                break;
        }
    }

    return buttons;
}

void Controller::ResetKeyboard(void)
{
}
