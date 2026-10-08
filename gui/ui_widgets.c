#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ui_widgets.h"
#include "ui_theme.h"

static Font uiFont;
static char *focusField;
static size_t cursorByte;
static size_t selectionByte;
static int mouseOverWidget;
static int controlCount;
static int previousControlCount;
static int keyboardControlIndex;
static int dropdownOpen;
static int *dropdownOwner;
static int dropdownScroll;
static int timePickerOpen;
static int *timePickerOwner;
static int timePickerScroll;
static int calendarOpen;
static char *calendarOwner;
static int calendarMonth;
static int calendarYear;

static int inside(Rectangle bounds)
{
    return CheckCollisionPointRec(GetMousePosition(), bounds);
}

static void drawRounded(Rectangle bounds, Color color)
{
    DrawRectangleRounded(bounds, 0.18f, 8, color);
}

static float measure(const char *text, float size)
{
    return MeasureTextEx(uiFont, text, size, 0.4f).x;
}

void uiWidgetsSetFont(Font font)
{
    uiFont = font;
}

void uiWidgetsBeginFrame(void)
{
    controlCount = 0;
    mouseOverWidget = 0;
    if (IsKeyPressed(KEY_TAB) && previousControlCount > 0) {
        keyboardControlIndex = (keyboardControlIndex + 1) % previousControlCount;
        focusField = NULL;
    }
}

void uiWidgetsEndFrame(void)
{
    previousControlCount = controlCount;
    if (keyboardControlIndex >= controlCount) keyboardControlIndex = 0;
    SetMouseCursor(mouseOverWidget ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

void uiLabel(const char *text, Vector2 position, float size, Color color)
{
    DrawTextEx(uiFont, text, position, size, 0.4f, color);
}

void uiPanel(Rectangle bounds, Color color)
{
    DrawRectangleRounded((Rectangle){bounds.x + 1, bounds.y + 4, bounds.width, bounds.height},
                         0.12f, 8, (Color){0, 0, 0, 36});
    drawRounded(bounds, color);
    DrawRectangleRoundedLinesEx(bounds, 0.12f, 8, 1.0f, UI_BORDER);
}

bool uiButton(Rectangle bounds, const char *label, UiButtonStyle style, bool enabled)
{
    int buttonIndex = controlCount++;
    bool hovered = enabled && inside(bounds);
    bool keyboardFocused = enabled && keyboardControlIndex == buttonIndex;
    Color background = UI_SURFACE_RAISED;
    Color foreground = UI_TEXT;

    if (style == UI_BUTTON_PRIMARY) background = UI_INDIGO;
    else if (style == UI_BUTTON_DANGER) background = (Color){127, 29, 29, 255};
    else if (style == UI_BUTTON_GHOST) background = (Color){0, 0, 0, 0};
    if (hovered && style != UI_BUTTON_GHOST) background = ColorBrightness(background, 0.13f);
    if (!enabled) {
        background = ColorAlpha(UI_SURFACE, 0.55f);
        foreground = ColorAlpha(UI_MUTED, 0.65f);
    }
    if (hovered) {
        mouseOverWidget = 1;
        keyboardControlIndex = buttonIndex;
    }
    if (style != UI_BUTTON_GHOST) drawRounded(bounds, background);
    else if (hovered) drawRounded(bounds, ColorAlpha(UI_SURFACE_RAISED, 0.8f));
    if (keyboardFocused) {
        DrawRectangleRoundedLinesEx(bounds, 0.18f, 8, 1.5f, UI_TEAL);
    }

    float textWidth = measure(label, UI_FONT_14);
    float textX = bounds.x + (bounds.width - textWidth) * 0.5f;
    float textY = bounds.y + (bounds.height - UI_FONT_14) * 0.5f - 1;
    uiLabel(label, (Vector2){textX, textY}, UI_FONT_14, foreground);
    return enabled && ((hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) ||
                       (keyboardFocused && IsKeyPressed(KEY_ENTER)));
}

static size_t previousCodepoint(const char *text, size_t position)
{
    if (position == 0) return 0;
    size_t previous = position - 1;
    while (previous > 0 && (((unsigned char)text[previous] & 0xC0) == 0x80)) previous--;
    return previous;
}

static size_t nextCodepoint(const char *text, size_t length, size_t position)
{
    if (position >= length) return length;
    size_t next = position + 1;
    while (next < length && (((unsigned char)text[next] & 0xC0) == 0x80)) next++;
    return next;
}

static void selectionRange(size_t *start, size_t *end)
{
    *start = cursorByte < selectionByte ? cursorByte : selectionByte;
    *end = cursorByte > selectionByte ? cursorByte : selectionByte;
}

static void replaceSelection(char *value, size_t capacity, const char *insert)
{
    size_t length = strlen(value), start, end;
    size_t insertLength = strlen(insert);
    selectionRange(&start, &end);
    if (length - (end - start) + insertLength >= capacity) return;
    memmove(value + start + insertLength, value + end, length - end + 1);
    memcpy(value + start, insert, insertLength);
    cursorByte = start + insertLength;
    selectionByte = cursorByte;
}

static int codepointWidth(int codepoint)
{
    return codepoint <= 0x7F ? 1 : codepoint <= 0x7FF ? 2 : codepoint <= 0xFFFF ? 3 : 4;
}

static void insertCodepoint(char *value, size_t capacity, int codepoint)
{
    if (codepoint < 32 || codepoint == 127) return;
    if (codepointWidth(codepoint) == 1 && (codepoint < 32 || codepoint > 126)) return;
    int encodedSize = 0;
    const char *encoded = CodepointToUTF8(codepoint, &encodedSize);
    if (encoded != NULL && encodedSize > 0 && encodedSize < 5) {
        char buffer[5] = {0};
        memcpy(buffer, encoded, (size_t)encodedSize);
        replaceSelection(value, capacity, buffer);
    }
}

bool uiTextInput(Rectangle bounds, char *value, size_t capacity,
                 const char *placeholder, bool numericOnly)
{
    int controlIndex = controlCount++;

    bool hovered = inside(bounds);
    bool keyboardFocused = keyboardControlIndex == controlIndex;
    if (hovered) mouseOverWidget = 1;
    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        focusField = value;
        keyboardControlIndex = controlIndex;
        cursorByte = strlen(value);
        selectionByte = cursorByte;
    }
    if (keyboardFocused) focusField = value;
    bool focused = focusField == value;
    Color background = focused ? UI_SURFACE_RAISED : UI_SURFACE;
    drawRounded(bounds, background);
    DrawRectangleRoundedLinesEx(bounds, 0.12f, 8, focused ? 1.5f : 1.0f,
                                focused ? UI_INDIGO : UI_BORDER);

    if (focused) {
        size_t length = strlen(value);
        bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        if (cursorByte > length) cursorByte = selectionByte = length;

        if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_A)) {
            selectionByte = 0;
            cursorByte = length;
        } else if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_C)) {
            size_t start, end;
            selectionRange(&start, &end);
            if (end > start) {
                char selected[256];
                size_t count = end - start;
                if (count >= sizeof(selected)) count = sizeof(selected) - 1;
                memcpy(selected, value + start, count);
                selected[count] = '\0';
                SetClipboardText(selected);
            }
        } else if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_X)) {
            size_t start, end;
            selectionRange(&start, &end);
            if (end > start) {
                char selected[256];
                size_t count = end - start;
                if (count >= sizeof(selected)) count = sizeof(selected) - 1;
                memcpy(selected, value + start, count);
                selected[count] = '\0';
                SetClipboardText(selected);
                replaceSelection(value, capacity, "");
            }
        } else if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_V)) {
            const char *clipboard = GetClipboardText();
            if (clipboard != NULL) {
                for (size_t i = 0; clipboard[i] != '\0'; i++) {
                    unsigned char byte = (unsigned char)clipboard[i];
                    if (numericOnly && !isdigit(byte) && byte != '/' && byte != ':' && byte != ',') continue;
                    if ((byte & 0xC0) == 0x80) continue;
                    if (byte < 0x80) insertCodepoint(value, capacity, byte);
                    else {
                        int codepoint = GetCodepoint(clipboard + i, NULL);
                        insertCodepoint(value, capacity, codepoint);
                        i += (size_t)codepointWidth(codepoint) - 1;
                    }
                }
            }
        } else if (IsKeyPressed(KEY_BACKSPACE)) {
            size_t start, end;
            selectionRange(&start, &end);
            if (start == end && start > 0) start = previousCodepoint(value, start);
            cursorByte = start;
            selectionByte = end;
            replaceSelection(value, capacity, "");
        } else if (IsKeyPressed(KEY_DELETE)) {
            size_t start, end;
            selectionRange(&start, &end);
            if (start == end) end = nextCodepoint(value, length, end);
            cursorByte = start;
            selectionByte = end;
            replaceSelection(value, capacity, "");
        } else if (IsKeyPressed(KEY_LEFT)) {
            cursorByte = cursorByte > 0 ? previousCodepoint(value, cursorByte) : 0;
            if (!shift) selectionByte = cursorByte;
        } else if (IsKeyPressed(KEY_RIGHT)) {
            cursorByte = nextCodepoint(value, length, cursorByte);
            if (!shift) selectionByte = cursorByte;
        } else if (IsKeyPressed(KEY_HOME)) {
            cursorByte = 0;
            if (!shift) selectionByte = cursorByte;
        } else if (IsKeyPressed(KEY_END)) {
            cursorByte = length;
            if (!shift) selectionByte = cursorByte;
        }

        int codepoint = GetCharPressed();
        while (codepoint > 0) {
            if (numericOnly) {
                int encodedSize = 0;
                const char *encoded = CodepointToUTF8(codepoint, &encodedSize);
                if (encoded != NULL && encodedSize > 0 &&
                    (isdigit((unsigned char)encoded[0]) || encoded[0] == '/' ||
                     encoded[0] == ':' || encoded[0] == ',')) {
                    insertCodepoint(value, capacity, codepoint);
                }
            } else {
                insertCodepoint(value, capacity, codepoint);
            }
            codepoint = GetCharPressed();
        }
    }

    const char *visible = value[0] == '\0' && !focused ? placeholder : value;
    Color textColor = value[0] == '\0' && !focused ? UI_MUTED : UI_TEXT;
    float textX = bounds.x + 12;
    float textY = bounds.y + (bounds.height - UI_FONT_14) * 0.5f - 1;
    if (focused && value[0] != '\0') {
        char before[256] = {0};
        size_t length = strlen(value);
        size_t keep = length < sizeof(before) - 1 ? length : sizeof(before) - 1;
        memcpy(before, value, keep);
        size_t start, end;
        selectionRange(&start, &end);
        float startX = textX + measure(before, UI_FONT_14) * ((float)start / (float)(keep ? keep : 1));
        float endX = textX + measure(before, UI_FONT_14) * ((float)end / (float)(keep ? keep : 1));
        if (end > start) DrawRectangle((int)startX, (int)(bounds.y + 8),
                                        (int)(endX - startX), (int)(bounds.height - 16),
                                        ColorAlpha(UI_INDIGO, 0.38f));
    }
    uiLabel(visible, (Vector2){textX, textY}, UI_FONT_14, textColor);
    if (focused && ((int)(GetTime() * 2) % 2 == 0)) {
        char before[256] = {0};
        size_t length = strlen(value);
        size_t count = cursorByte < sizeof(before) - 1 ? cursorByte : sizeof(before) - 1;
        if (count > length) count = length;
        memcpy(before, value, count);
        float x = textX + measure(before, UI_FONT_14);
        DrawLineEx((Vector2){x, bounds.y + 11}, (Vector2){x, bounds.y + bounds.height - 11},
                   1.2f, UI_TEAL);
    }
    return focused && IsKeyPressed(KEY_ENTER);
}

bool uiCheckbox(Rectangle bounds, const char *label, bool *checked)
{
    Rectangle box = {bounds.x, bounds.y + (bounds.height - 18) * 0.5f, 18, 18};
    bool hovered = inside(bounds);
    if (hovered) mouseOverWidget = 1;
    DrawRectangleRounded(box, 0.22f, 5, *checked ? UI_INDIGO : UI_SURFACE);
    DrawRectangleRoundedLinesEx(box, 0.2f, 5, 1.0f, *checked ? UI_INDIGO : UI_BORDER);
    if (*checked) uiLabel("v", (Vector2){box.x + 4, box.y + 1}, UI_FONT_12, UI_WHITE);
    uiLabel(label, (Vector2){bounds.x + 27, bounds.y + (bounds.height - UI_FONT_14) * 0.5f},
            UI_FONT_14, UI_TEXT);
    if (hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        *checked = !*checked;
        return true;
    }
    return false;
}

bool uiDropdown(Rectangle bounds, const char *const *items, size_t count,
                int *selected)
{
    char label[160];
    const char *text = count > 0 && *selected >= 0 && (size_t)*selected < count
        ? items[*selected] : "Selectionner";
    snprintf(label, sizeof(label), "%s  v", text);
    bool changed = false;
    if (uiButton(bounds, label, UI_BUTTON_SECONDARY, true)) {
        dropdownOwner = selected;
        dropdownOpen = !dropdownOpen || dropdownOwner != selected;
        dropdownScroll = 0;
    }
    if (dropdownOpen && dropdownOwner == selected) {
        int visible = count > 6 ? 6 : (int)count;
        Rectangle popup = {bounds.x, bounds.y + bounds.height + 4, bounds.width,
                           visible * 34.0f};
        uiPanel(popup, UI_SURFACE_RAISED);
        for (int i = 0; i < visible; i++) {
            int item = dropdownScroll + i;
            Rectangle row = {popup.x + 4, popup.y + 4 + i * 34.0f,
                             popup.width - 8, 30};
            if ((size_t)item < count && uiButton(row, items[item],
                    item == *selected ? UI_BUTTON_PRIMARY : UI_BUTTON_GHOST, true)) {
                *selected = item;
                dropdownOpen = false;
                changed = true;
            }
        }
        if (inside(popup)) {
            mouseOverWidget = 1;
            float wheel = GetMouseWheelMove();
            if (wheel < 0 && dropdownScroll + visible < (int)count) dropdownScroll++;
            if (wheel > 0 && dropdownScroll > 0) dropdownScroll--;
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !inside(bounds)) {
            dropdownOpen = false;
        }
    }
    return changed;
}

static void decodeDate(const char *value, int *day, int *month, int *year)
{
    if (sscanf(value, "%2d/%2d/%4d", day, month, year) != 3 ||
        *month < 1 || *month > 12 || *day < 1 || *day > 31 || *year < 1) {
        time_t now = time(NULL);
        struct tm *local = localtime(&now);
        *day = local->tm_mday;
        *month = local->tm_mon + 1;
        *year = local->tm_year + 1900;
    }
}

bool uiDatePicker(Rectangle bounds, char *value, size_t capacity)
{
    bool changed = false;
    Rectangle textBounds = {bounds.x, bounds.y, bounds.width - 42, bounds.height};
    uiTextInput(textBounds, value, capacity, "JJ/MM/AAAA", true);
    Rectangle calendarButton = {bounds.x + bounds.width - 36, bounds.y, 36, bounds.height};
    if (uiButton(calendarButton, "D", UI_BUTTON_SECONDARY, true)) {
        if (!calendarOpen || calendarOwner != value) {
            int day, month, year;
            decodeDate(value, &day, &month, &year);
            calendarMonth = month;
            calendarYear = year;
            calendarOwner = value;
            calendarOpen = 1;
        } else {
            calendarOpen = 0;
        }
    }
    if (!calendarOpen || calendarOwner != value) return changed;

    Rectangle panel = {bounds.x, bounds.y + bounds.height + 6, 276, 286};
    uiPanel(panel, UI_SURFACE_RAISED);
    char monthLabel[64];
    static const char *mois[] = {"Janvier","Fevrier","Mars","Avril","Mai","Juin",
                                 "Juillet","Aout","Septembre","Octobre","Novembre","Decembre"};
    snprintf(monthLabel, sizeof(monthLabel), "%s %d", mois[calendarMonth - 1], calendarYear);
    uiLabel(monthLabel, (Vector2){panel.x + 56, panel.y + 14}, UI_FONT_14, UI_TEXT);
    if (uiButton((Rectangle){panel.x + 8, panel.y + 8, 34, 28}, "<", UI_BUTTON_GHOST, true)) {
        if (--calendarMonth < 1) { calendarMonth = 12; calendarYear--; }
    }
    if (uiButton((Rectangle){panel.x + panel.width - 42, panel.y + 8, 34, 28},
                 ">", UI_BUTTON_GHOST, true)) {
        if (++calendarMonth > 12) { calendarMonth = 1; calendarYear++; }
    }

    static const char *jours[] = {"L","M","M","J","V","S","D"};
    for (int i = 0; i < 7; i++) {
        uiLabel(jours[i], (Vector2){panel.x + 12 + i * 36, panel.y + 48},
                UI_FONT_12, UI_MUTED);
    }
    struct tm premier = {0};
    premier.tm_year = calendarYear - 1900;
    premier.tm_mon = calendarMonth - 1;
    premier.tm_mday = 1;
    mktime(&premier);
    int decalage = (premier.tm_wday + 6) % 7;
    static const int joursMois[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int nombreJours = joursMois[calendarMonth - 1];
    if (calendarMonth == 2 &&
        (calendarYear % 400 == 0 || (calendarYear % 4 == 0 && calendarYear % 100 != 0))) {
        nombreJours = 29;
    }
    for (int jour = 1; jour <= nombreJours; jour++) {
        int cellule = decalage + jour - 1;
        int col = cellule % 7;
        int ligne = cellule / 7;
        Rectangle celluleBounds = {panel.x + 7 + col * 37, panel.y + 70 + ligne * 34, 32, 30};
        if (uiButton(celluleBounds, TextFormat("%d", jour), UI_BUTTON_GHOST, true)) {
            snprintf(value, capacity, "%02d/%02d/%04d", jour, calendarMonth, calendarYear);
            calendarOpen = 0;
            changed = true;
        }
    }
    if (inside(panel)) mouseOverWidget = 1;
    return changed;
}

static void formatTime(int minutes, char output[8])
{
    if (minutes < 0) minutes = 0;
    if (minutes > 1439) minutes = 1439;
    snprintf(output, 8, "%02d:%02d", minutes / 60, minutes % 60);
}

bool uiTimePicker(Rectangle bounds, int *minutes)
{
    char label[16];
    formatTime(*minutes, label);
    int changed = 0;
    if (uiButton(bounds, label, UI_BUTTON_SECONDARY, true)) {
        if (!timePickerOpen || timePickerOwner != minutes) {
            timePickerOwner = minutes;
            timePickerScroll = 0;
            timePickerOpen = 1;
        } else {
            timePickerOpen = 0;
        }
    }
    if (!timePickerOpen || timePickerOwner != minutes) return false;

    int options[33], count = 0;
    for (int minute = 8 * 60; minute <= 23 * 60 + 30; minute += 30) options[count++] = minute;
    options[count++] = 23 * 60 + 59;
    int visible = 7;
    Rectangle popup = {bounds.x, bounds.y + bounds.height + 4, bounds.width, visible * 32.0f};
    uiPanel(popup, UI_SURFACE_RAISED);
    for (int i = 0; i < visible; i++) {
        int index = timePickerScroll + i;
        if (index >= count) break;
        char timeLabel[8];
        formatTime(options[index], timeLabel);
        Rectangle row = {popup.x + 4, popup.y + 4 + i * 32.0f,
                         popup.width - 8, 28};
        if (uiButton(row, timeLabel, UI_BUTTON_GHOST, true)) {
            *minutes = options[index];
            timePickerOpen = 0;
            changed = 1;
        }
    }
    if (inside(popup)) {
        mouseOverWidget = 1;
        float wheel = GetMouseWheelMove();
        if (wheel < 0 && timePickerScroll + visible < count) timePickerScroll++;
        if (wheel > 0 && timePickerScroll > 0) timePickerScroll--;
    } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !inside(bounds)) {
        timePickerOpen = 0;
    }
    return changed != 0;
}

void uiTooltip(Rectangle bounds, const char *text)
{
    if (!inside(bounds)) return;
    float width = measure(text, UI_FONT_12) + 20;
    Rectangle tip = {GetMouseX() + 12, GetMouseY() + 16, width, 30};
    uiPanel(tip, UI_SURFACE_RAISED);
    uiLabel(text, (Vector2){tip.x + 10, tip.y + 8}, UI_FONT_12, UI_TEXT);
}

void uiToast(const char *message, Color color, float remainingSeconds)
{
    if (remainingSeconds <= 0.0f) return;
    float width = measure(message, UI_FONT_14) + 40;
    Rectangle bounds = {GetScreenWidth() - width - 28, 22, width, 48};
    drawRounded(bounds, UI_SURFACE_RAISED);
    DrawRectangleRounded((Rectangle){bounds.x, bounds.y, 4, bounds.height},
                         0.8f, 8, color);
    uiLabel(message, (Vector2){bounds.x + 18, bounds.y + 16}, UI_FONT_14, UI_TEXT);
}
