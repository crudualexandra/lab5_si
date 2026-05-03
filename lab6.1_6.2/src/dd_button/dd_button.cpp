#include <Arduino.h>
#include "dd_button.h"

typedef struct {
    int pin;
    bool currentState;
    bool lastStableState;
    bool justPressed;
    bool justReleased;
    unsigned long lastDebounceTime;
} ButtonState;

static ButtonState buttons[DD_BUTTON_MAX_COUNT];

void hwButtonInit(int btnId, int pin)
{
    if (btnId < 0 || btnId >= DD_BUTTON_MAX_COUNT)
    {
        return;
    }

    buttons[btnId].pin = pin;
    buttons[btnId].currentState = false;
    buttons[btnId].lastStableState = false;
    buttons[btnId].justPressed = false;
    buttons[btnId].justReleased = false;
    buttons[btnId].lastDebounceTime = 0;

    pinMode(pin, INPUT_PULLUP);
}

void hwButtonUpdate(int btnId)
{
    if (btnId < 0 || btnId >= DD_BUTTON_MAX_COUNT)
    {
        return;
    }

    ButtonState* b = &buttons[btnId];

    bool rawPressed = (digitalRead(b->pin) == LOW);

    b->justPressed = false;
    b->justReleased = false;

    if (rawPressed != b->currentState)
    {
        b->lastDebounceTime = millis();
        b->currentState = rawPressed;
    }

    if ((millis() - b->lastDebounceTime) >= DD_BUTTON_DEBOUNCE_MS)
    {
        if (b->currentState != b->lastStableState)
        {
            if (b->currentState && !b->lastStableState)
            {
                b->justPressed = true;
            }
            else if (!b->currentState && b->lastStableState)
            {
                b->justReleased = true;
            }

            b->lastStableState = b->currentState;
        }
    }
}

bool hwButtonIsPressed(int btnId)
{
    if (btnId < 0 || btnId >= DD_BUTTON_MAX_COUNT)
    {
        return false;
    }

    return buttons[btnId].lastStableState;
}

bool hwButtonPressed(int btnId)
{
    if (btnId < 0 || btnId >= DD_BUTTON_MAX_COUNT)
    {
        return false;
    }

    bool val = buttons[btnId].justPressed;
    buttons[btnId].justPressed = false;
    return val;
}

bool hwButtonReleased(int btnId)
{
    if (btnId < 0 || btnId >= DD_BUTTON_MAX_COUNT)
    {
        return false;
    }

    bool val = buttons[btnId].justReleased;
    buttons[btnId].justReleased = false;
    return val;
}
