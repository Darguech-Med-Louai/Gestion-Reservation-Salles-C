#ifndef ORBITE_UI_WIDGETS_H
#define ORBITE_UI_WIDGETS_H

#include <stdbool.h>
#include <stddef.h>
#include "raylib.h"

typedef enum {
    UI_BUTTON_PRIMARY,
    UI_BUTTON_SECONDARY,
    UI_BUTTON_GHOST,
    UI_BUTTON_DANGER
} UiButtonStyle;

void uiWidgetsSetFont(Font font);
void uiWidgetsBeginFrame(void);
void uiWidgetsEndFrame(void);
bool uiButton(Rectangle bounds, const char *label, UiButtonStyle style, bool enabled);
bool uiTextInput(Rectangle bounds, char *value, size_t capacity,
                 const char *placeholder, bool numericOnly);
bool uiCheckbox(Rectangle bounds, const char *label, bool *checked);
bool uiDropdown(Rectangle bounds, const char *const *items, size_t count,
                int *selected);
bool uiDatePicker(Rectangle bounds, char *value, size_t capacity);
bool uiTimePicker(Rectangle bounds, int *minutes);
void uiLabel(const char *text, Vector2 position, float size, Color color);
void uiPanel(Rectangle bounds, Color color);
void uiTooltip(Rectangle bounds, const char *text);
void uiToast(const char *message, Color color, float remainingSeconds);

#endif
