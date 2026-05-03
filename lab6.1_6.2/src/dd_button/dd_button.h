#ifndef DD_BUTTON_H
#define DD_BUTTON_H

#define DD_BUTTON_MAX_COUNT 4
#define DD_BUTTON_DEBOUNCE_MS 20

void hwButtonInit(int btnId, int pin);
bool hwButtonIsPressed(int btnId);
bool hwButtonPressed(int btnId);
bool hwButtonReleased(int btnId);
void hwButtonUpdate(int btnId);

#endif
