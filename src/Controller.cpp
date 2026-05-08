#include "Controller.hpp"

#include <SDL_events.h>
#include <SDL_keyboard.h>

#include "GameErrorContext.hpp"
#include "Supervisor.hpp"
#include "SDLCompat.hpp"
#include "i18n.hpp"
#include "utils.hpp"

// DIFFABLE_STATIC(JOYCAPSA, g_JoystickCaps)
static u16 g_FocusButtonConflictState;
static u8 *keyboardState;

u16 Controller::GetJoystickCaps(void)
{
    //    JOYINFOEX pji;

    //    pji.dwSize = sizeof(JOYINFOEX);
    //    pji.dwFlags = JOY_RETURNALL;

    //    if (joyGetPosEx(0, &pji) != MMSYSERR_NOERROR)
    //    {
    //        GameErrorContext::Log(&g_GameErrorContext, TH_ERR_NO_PAD_FOUND);
    //        return 1;
    //    }
    //
    //    joyGetDevCapsA(0, &g_JoystickCaps, sizeof(g_JoystickCaps));
    return 0;
}

#define JOYSTICK_MIDPOINT(min, max) ((min + max) / 2)
#define JOYSTICK_BUTTON_PRESSED(button, x, y) (x > y ? button : 0)
#define JOYSTICK_BUTTON_PRESSED_INVERT(button, x, y) (x < y ? button : 0)
#define KEYBOARD_KEY_PRESSED(button, x) keyboardState[x] ? button : 0

u16 Controller::GetControllerInput(u16 buttons)
{
    i16 stickX = 0;
    i16 stickY = 0;
    u32 shootPressed = 0;

    if (g_Supervisor.joystick != NULL)
    {
        // -------------------------
        // Button mapping
        // -------------------------
        shootPressed = SetButtonFromControllerInputs(
            &buttons,
            g_Supervisor.cfg.controllerMapping.shootButton,
            TH_BUTTON_SHOOT,
            g_Supervisor.joystick
        );

        if (g_Supervisor.cfg.controllerMapping.shootButton !=
            g_Supervisor.cfg.controllerMapping.focusButton)
        {
            SetButtonFromControllerInputs(
                &buttons,
                g_Supervisor.cfg.controllerMapping.focusButton,
                TH_BUTTON_FOCUS,
                g_Supervisor.joystick
            );
        }
        else
        {
            if (shootPressed)
            {
                if (g_FocusButtonConflictState < 16)
                    g_FocusButtonConflictState++;

                if (g_FocusButtonConflictState >= 8)
                    buttons |= TH_BUTTON_FOCUS;
            }
            else
            {
                if (g_FocusButtonConflictState > 8)
                    g_FocusButtonConflictState -= 8;
                else
                    g_FocusButtonConflictState = 0;
            }
        }

        SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.bombButton, TH_BUTTON_BOMB, g_Supervisor.joystick);
        SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.menuButton, TH_BUTTON_MENU, g_Supervisor.joystick);
        SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.upButton, TH_BUTTON_UP, g_Supervisor.joystick);
        SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.downButton, TH_BUTTON_DOWN, g_Supervisor.joystick);
        SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.leftButton, TH_BUTTON_LEFT, g_Supervisor.joystick);
        SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.rightButton, TH_BUTTON_RIGHT, g_Supervisor.joystick);
        SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.skipButton, TH_BUTTON_SKIP, g_Supervisor.joystick);

        // -------------------------
        // SDL 1.2 axes (ONLY 0 and 1)
        // -------------------------
        stickX = SDL_JoystickGetAxis(g_Supervisor.joystick, 0);
        stickY = SDL_JoystickGetAxis(g_Supervisor.joystick, 1);

        const int DEADZONE = 16000;

        if (stickX > DEADZONE)
            buttons |= TH_BUTTON_RIGHT;
        if (stickX < -DEADZONE)
            buttons |= TH_BUTTON_LEFT;

        if (stickY > DEADZONE)
            buttons |= TH_BUTTON_DOWN;
        if (stickY < -DEADZONE)
            buttons |= TH_BUTTON_UP;
    }

    return buttons;
}

u32 Controller::SetButtonFromDirectInputJoystate(u16 *outButtons, i16 controllerButtonToTest,
                                                 enum TouhouButton touhouButton, const u8 *inputButtons)
{
    if (controllerButtonToTest < 0)
    {
        return 0;
    }

    *outButtons |= (inputButtons[controllerButtonToTest] & 0x80 ? touhouButton & 0xFFFF : 0);

    return inputButtons[controllerButtonToTest] & 0x80 ? touhouButton & 0xFFFF : 0;
}

u32 Controller::SetButtonFromControllerInputs(u16 *outButtons, i16 controllerButtonToTest,
                                              enum TouhouButton touhouButton, SDL_Joystick *controller)
{
    u8 pressed;

    if (controllerButtonToTest < 0)
    {
        return 0;
    }

    pressed = SDL_JoystickGetButton(controller, controllerButtonToTest);

    *outButtons |= pressed ? touhouButton & 0xFFFF : 0;

    return pressed ? touhouButton & 0xFFFF : 0;
}

static u8 g_ControllerData[SDL_CONTROLLER_BUTTON_MAX];

// This is for rebinding keys
const u8 *Controller::GetControllerState()
{
    if (g_Supervisor.joystick != NULL)
    {
        memset(&g_ControllerData, 0, sizeof(g_ControllerData));

        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
        {
            if (SDL_JoystickGetButton(g_Supervisor.joystick, i))
            {
                g_ControllerData[i] = 0x80;
            }
        }
    }
    return g_ControllerData;
}

u16 Controller::GetInput(void)
{
    Uint8 *keyboardState = SDL_GetKeyState(NULL);
    SDL_PumpEvents();
    u16 buttons = 0;

    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP, SDLK_UP);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN, SDLK_DOWN);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_LEFT, SDLK_LEFT);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_RIGHT, SDLK_RIGHT);

    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP, SDLK_KP8);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN, SDLK_KP2);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_LEFT, SDLK_KP4);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_RIGHT, SDLK_KP6);

    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP_LEFT, SDLK_KP7);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP_RIGHT, SDLK_KP9);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN_LEFT, SDLK_KP1);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN_RIGHT, SDLK_KP3);

    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_HOME, SDLK_HOME);

    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_SHOOT, SDLK_z);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_BOMB, SDLK_x);

    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_FOCUS, SDLK_LSHIFT);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_FOCUS, SDLK_RSHIFT);

    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_MENU, SDLK_ESCAPE);

    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_SKIP, SDLK_LCTRL);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_SKIP, SDLK_RCTRL);

    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_Q, SDLK_q);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_S, SDLK_s);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_ENTER, SDLK_RETURN);

    return Controller::GetControllerInput(buttons);
}
void Controller::ResetKeyboard(void)
{
}