#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"
#include "ui_theme.h"
#include "ui_widgets.h"
#include "../src/core/Core.h"
#include "../src/FonctionsAux.h"

#define SIDEBAR_WIDTH 224
#define HEADER_HEIGHT 88
#define MAX_VISIBLE_ROWS 100

typedef enum {
    PAGE_DASHBOARD,
    PAGE_ROOMS,
    PAGE_BOOKING,
    PAGE_RESERVATIONS,
    PAGE_PLANNING,
    PAGE_INVOICES,
    PAGE_COUNT
} Page;

typedef enum {
    MODAL_NONE,
    MODAL_ROOM_FORM,
    MODAL_DELETE_ROOM,
    MODAL_RESERVATION_ACTION
} Modal;

typedef enum {
    ACTION_NONE,
    ACTION_CANCEL_RESERVATION,
    ACTION_DELETE_RESERVATION
} ReservationAction;

typedef struct {
    Page page;
    Modal modal;
    ReservationAction action;
    Font font;
    Texture2D brandMark;
    char search[80];
    bool searchFocused;
    int periodMonth;
    int periodYear;
    bool allPeriods;
    int selectedReservation;
    int selectedInvoice;
    int selectedRoom;
    int editingRoom;
    int editingReservation;
    int roomCapacity;
    int personCount;
    int roomFilter;
    int statusFilter;
    int sortDescending;
    int roomScroll;
    int reservationScroll;
    int plannerScroll;
    int invoiceScroll;
    int wizardStep;
    int selectedWizardRoom;
    int startMinutes;
    int endMinutes;
    char bookingClient[50];
    char bookingPeople[12];
    char bookingDate[11];
    char reservationDateFilter[11];
    char plannerDate[11];
    char roomName[50];
    char roomCapacityText[12];
    char roomRateText[20];
    bool roomWifi;
    bool roomProjector;
    bool roomWhiteboard;
    float toastSeconds;
    Color toastColor;
    char toastMessage[180];
    bool captureScreenshots;
    bool captureExit;
    int captureIndex;
    int captureDelayFrames;
} AppState;

static AppState app;

static float clampf(float value, float minValue, float maxValue)
{
    return value < minValue ? minValue : value > maxValue ? maxValue : value;
}

static int inside(Rectangle bounds)
{
    return CheckCollisionPointRec(GetMousePosition(), bounds);
}

static void drawRounded(Rectangle bounds, Color color)
{
    DrawRectangleRounded(bounds, 0.16f, 8, color);
}

static const char *pageTitles[PAGE_COUNT] = {
    "Vue d'ensemble", "Salles", "Nouvelle reservation",
    "Reservations", "Planning", "Factures"
};
static const char *navLabels[PAGE_COUNT] = {
    "Vue d'ensemble", "Salles", "Nouvelle reservation",
    "Reservations", "Planning", "Factures"
};
static const char *monthNames[12] = {
    "Janvier", "Fevrier", "Mars", "Avril", "Mai", "Juin",
    "Juillet", "Aout", "Septembre", "Octobre", "Novembre", "Decembre"
};

static void notify(const char *message, Color color)
{
    snprintf(app.toastMessage, sizeof(app.toastMessage), "%s", message);
    app.toastColor = color;
    app.toastSeconds = 3.2f;
}

static void notifyResult(ResCode code)
{
    notify(resCodeMessage(code), code == RES_OK ? UI_SUCCESS : UI_ERROR);
}

static void moneyText(float amount, char *destination, size_t capacity)
{
    snprintf(destination, capacity, "%.2f", amount);
    for (size_t i = 0; destination[i] != '\0'; i++) {
        if (destination[i] == '.') destination[i] = ',';
    }
}

static int parsePositiveInt(const char *text, int *value)
{
    char *end;
    long parsed;
    if (text == NULL || text[0] == '\0') return 0;
    parsed = strtol(text, &end, 10);
    if (end == text || *end != '\0' || parsed <= 0 || parsed > 100000) return 0;
    *value = (int)parsed;
    return 1;
}

static int parseMoney(const char *text, float *value)
{
    char copy[32];
    char *end;
    if (text == NULL || text[0] == '\0' || strlen(text) >= sizeof(copy)) return 0;
    strcpy(copy, text);
    for (size_t i = 0; copy[i] != '\0'; i++) {
        if (copy[i] == ',') copy[i] = '.';
    }
    *value = strtof(copy, &end);
    return end != copy && *end == '\0' && isfinite(*value) && *value >= 0.0f;
}

static void todayText(char value[11])
{
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    if (local == NULL || strftime(value, 11, "%d/%m/%Y", local) == 0) {
        snprintf(value, 11, "01/01/1970");
    }
}

static void dateParts(const char *date, int *day, int *month, int *year)
{
    *day = *month = *year = 0;
    if (sscanf(date, "%2d/%2d/%4d", day, month, year) != 3) {
        char today[11];
        todayText(today);
        (void)sscanf(today, "%2d/%2d/%4d", day, month, year);
    }
}

static void shiftDate(char date[11], int offsetDays)
{
    int day, month, year;
    dateParts(date, &day, &month, &year);
    struct tm value = {0};
    value.tm_mday = day + offsetDays;
    value.tm_mon = month - 1;
    value.tm_year = year - 1900;
    value.tm_isdst = -1;
    if (mktime(&value) != (time_t)-1) {
        strftime(date, 11, "%d/%m/%Y", &value);
    }
}

static int compareDate(const char *left, const char *right)
{
    int ld, lm, ly, rd, rm, ry;
    dateParts(left, &ld, &lm, &ly);
    dateParts(right, &rd, &rm, &ry);
    if (ly != ry) return ly < ry ? -1 : 1;
    if (lm != rm) return lm < rm ? -1 : 1;
    if (ld != rd) return ld < rd ? -1 : 1;
    return 0;
}

static int parseMonth(const char *date, int *month, int *year)
{
    int day;
    if (sscanf(date, "%2d/%2d/%4d", &day, month, year) != 3 ||
        day < 1 || *month < 1 || *month > 12 || *year < 1) {
        return 0;
    }
    return 1;
}

static bool containsText(const char *text, const char *query)
{
    if (query == NULL || query[0] == '\0') return true;
    if (text == NULL) return false;
    size_t n = strlen(query);
    for (const char *p = text; *p != '\0'; p++) {
        size_t i = 0;
        while (i < n && p[i] != '\0' &&
               tolower((unsigned char)p[i]) == tolower((unsigned char)query[i])) {
            i++;
        }
        if (i == n) return true;
    }
    return false;
}

static void drawTextWrapped(const char *text, float x, float y, float maxWidth,
                            float size, Color color)
{
    char line[384] = {0};
    char word[128];
    size_t wordLength = 0;
    const char *p = text;
    float lineY = y;

    while (1) {
        if (*p == ' ' || *p == '\n' || *p == '\0') {
            word[wordLength] = '\0';
            if (wordLength > 0) {
                char candidate[384];
                size_t lineLength = strlen(line);
                size_t separatorLength = lineLength > 0 ? 1 : 0;
                size_t copiedLine = lineLength < sizeof(candidate) - 1
                    ? lineLength : sizeof(candidate) - 1;
                memcpy(candidate, line, copiedLine);
                size_t offset = copiedLine;
                if (separatorLength && offset < sizeof(candidate) - 1) {
                    candidate[offset++] = ' ';
                }
                size_t wordCopy = wordLength < sizeof(candidate) - offset - 1
                    ? wordLength : sizeof(candidate) - offset - 1;
                memcpy(candidate + offset, word, wordCopy);
                candidate[offset + wordCopy] = '\0';
                if (MeasureTextEx(app.font, candidate, size, 0.4f).x > maxWidth &&
                    line[0] != '\0') {
                    uiLabel(line, (Vector2){x, lineY}, size, color);
                    lineY += size + 5;
                    snprintf(line, sizeof(line), "%s", word);
                } else {
                    snprintf(line, sizeof(line), "%s", candidate);
                }
                wordLength = 0;
            }
            if (*p == '\n' || *p == '\0') {
                if (line[0] != '\0') uiLabel(line, (Vector2){x, lineY}, size, color);
                line[0] = '\0';
                lineY += size + 5;
            }
            if (*p == '\0') break;
        } else if (wordLength + 1 < sizeof(word)) {
            word[wordLength++] = *p;
        }
        p++;
    }
}

static void drawIcon(int index, float x, float y, Color color)
{
    switch (index) {
        case PAGE_DASHBOARD:
            DrawRectangleRounded((Rectangle){x, y, 8, 8}, 0.2f, 3, color);
            DrawRectangleRounded((Rectangle){x + 11, y, 8, 8}, 0.2f, 3, color);
            DrawRectangleRounded((Rectangle){x, y + 11, 8, 8}, 0.2f, 3, color);
            DrawRectangleRounded((Rectangle){x + 11, y + 11, 8, 8}, 0.2f, 3, color);
            break;
        case PAGE_ROOMS:
            DrawRectangleLinesEx((Rectangle){x + 1, y + 1, 18, 18}, 1.5f, color);
            DrawLineEx((Vector2){x + 5, y + 7}, (Vector2){x + 9, y + 7}, 1.5f, color);
            DrawLineEx((Vector2){x + 12, y + 7}, (Vector2){x + 16, y + 7}, 1.5f, color);
            DrawLineEx((Vector2){x + 5, y + 12}, (Vector2){x + 9, y + 12}, 1.5f, color);
            break;
        case PAGE_BOOKING:
            DrawCircleLines((int)(x + 10), (int)(y + 10), 9, color);
            DrawLineEx((Vector2){x + 10, y + 5}, (Vector2){x + 10, y + 15}, 2, color);
            DrawLineEx((Vector2){x + 5, y + 10}, (Vector2){x + 15, y + 10}, 2, color);
            break;
        case PAGE_RESERVATIONS:
            for (int row = 0; row < 3; row++) {
                DrawCircle((int)(x + 3), (int)(y + 5 + row * 6), 1.5f, color);
                DrawLineEx((Vector2){x + 8, y + 5 + row * 6},
                           (Vector2){x + 19, y + 5 + row * 6}, 1.5f, color);
            }
            break;
        case PAGE_PLANNING:
            DrawRectangleLinesEx((Rectangle){x + 1, y + 2, 18, 17}, 1.5f, color);
            DrawLineEx((Vector2){x + 2, y + 7}, (Vector2){x + 18, y + 7}, 1.5f, color);
            DrawLineEx((Vector2){x + 6, y}, (Vector2){x + 6, y + 5}, 2, color);
            DrawLineEx((Vector2){x + 14, y}, (Vector2){x + 14, y + 5}, 2, color);
            break;
        default:
            DrawRectangleLinesEx((Rectangle){x + 3, y + 1, 15, 19}, 1.5f, color);
            DrawLineEx((Vector2){x + 6, y + 7}, (Vector2){x + 15, y + 7}, 1.2f, color);
            DrawLineEx((Vector2){x + 6, y + 11}, (Vector2){x + 15, y + 11}, 1.2f, color);
            DrawLineEx((Vector2){x + 6, y + 15}, (Vector2){x + 12, y + 15}, 1.2f, color);
            break;
    }
}

static void startBooking(int reservationId)
{
    app.page = PAGE_BOOKING;
    app.editingReservation = reservationId;
    app.wizardStep = 0;
    app.selectedWizardRoom = -1;
    app.bookingPeople[0] = '\0';
    if (reservationId > 0) {
        Reservation *reservation = resTrouver(reservationId);
        if (reservation != NULL) {
            snprintf(app.bookingClient, sizeof(app.bookingClient), "%s",
                     reservation->nom_client);
            snprintf(app.bookingPeople, sizeof(app.bookingPeople), "%d",
                     reservation->nombre_personnes);
            snprintf(app.bookingDate, sizeof(app.bookingDate), "%s", reservation->date);
            app.startMinutes = heureEnMinutes(reservation->heure_debut);
            app.endMinutes = heureEnMinutes(reservation->heure_fin);
            for (int i = 0; i < nb_salles; i++) {
                if (strcmp(salles[i].nom, reservation->salle.nom) == 0) {
                    app.selectedWizardRoom = i;
                    break;
                }
            }
        }
    } else {
        app.bookingClient[0] = '\0';
        todayText(app.bookingDate);
        app.startMinutes = 9 * 60;
        app.endMinutes = 10 * 60;
    }
}

static void initializePeriod(void)
{
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    app.periodMonth = local->tm_mon + 1;
    app.periodYear = local->tm_year + 1900;

    bool foundPeriod = false;
    for (int i = 0; i < nb_reservations; i++) {
        int month, year;
        if (strcmp(reservations[i].statut, "annulee") != 0 &&
            parseMonth(reservations[i].date, &month, &year) &&
            (!foundPeriod || year > app.periodYear ||
             (year == app.periodYear && month > app.periodMonth))) {
            if (!foundPeriod || year > app.periodYear ||
                (year == app.periodYear && month > app.periodMonth)) {
                app.periodMonth = month;
                app.periodYear = year;
            }
            foundPeriod = true;
        }
    }
    app.selectedReservation = nb_reservations > 0 ? reservations[nb_reservations - 1].id : -1;
    app.selectedInvoice = app.selectedReservation;
    todayText(app.plannerDate);
}

static void drawSidebar(void)
{
    Rectangle sidebar = {0, 0, SIDEBAR_WIDTH, (float)GetScreenHeight()};
    DrawRectangleRec(sidebar, (Color){17, 25, 45, 255});
    DrawLineEx((Vector2){SIDEBAR_WIDTH - 1, 0},
               (Vector2){SIDEBAR_WIDTH - 1, (float)GetScreenHeight()}, 1, UI_BORDER);

    DrawTextureEx(app.brandMark, (Vector2){24, 24}, 0.0f, 42.0f / app.brandMark.width,
                  UI_WHITE);
    uiLabel("ORBITE", (Vector2){78, 25}, UI_FONT_20, UI_TEXT);
    uiLabel("GESTION DES ESPACES", (Vector2){78, 52}, UI_FONT_12, UI_MUTED);

    uiLabel("ESPACE DE TRAVAIL", (Vector2){24, 106}, UI_FONT_12, UI_MUTED);
    for (int i = 0; i < PAGE_COUNT; i++) {
        float y = 132 + i * 54.0f;
        Rectangle item = {14, y, SIDEBAR_WIDTH - 28.0f, 46};
        bool active = app.page == (Page)i;
        bool hovered = CheckCollisionPointRec(GetMousePosition(), item);
        if (active) {
            drawRounded(item, ColorAlpha(UI_INDIGO, 0.22f));
            DrawRectangleRounded((Rectangle){14, y + 8, 3, 30}, 0.8f, 5, UI_INDIGO);
        } else if (hovered) {
            drawRounded(item, ColorAlpha(UI_SURFACE_RAISED, 0.65f));
        }
        Color color = active ? UI_TEXT : UI_MUTED;
        drawIcon(i, 29, y + 13, active ? UI_TEAL : color);
        uiLabel(navLabels[i], (Vector2){61, y + 15}, UI_FONT_14, color);
        if (uiButton(item, "", UI_BUTTON_GHOST, true)) {
            if (i == PAGE_BOOKING) startBooking(-1);
            else app.page = (Page)i;
        }
    }

    float bottom = (float)GetScreenHeight() - 68;
    DrawLineEx((Vector2){24, bottom - 12}, (Vector2){SIDEBAR_WIDTH - 24, bottom - 12},
               1, UI_BORDER);
    uiLabel("ORBITE DESKTOP", (Vector2){24, bottom}, UI_FONT_12, UI_MUTED);
    uiLabel("Version 1.0  |  raylib", (Vector2){24, bottom + 20}, UI_FONT_12,
            ColorAlpha(UI_MUTED, 0.72f));
}

static void drawHeader(void)
{
    DrawRectangle( SIDEBAR_WIDTH, 0, GetScreenWidth() - SIDEBAR_WIDTH,
                   HEADER_HEIGHT, UI_BG);
    uiLabel(pageTitles[app.page], (Vector2){252, 22}, UI_FONT_28, UI_TEXT);
    uiLabel("Votre espace de reservation, en un seul endroit",
            (Vector2){253, 57}, UI_FONT_12, UI_MUTED);

    float right = (float)GetScreenWidth() - 28;
    char dateText[80];
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    strftime(dateText, sizeof(dateText), "%d/%m/%Y", local);
    float dateWidth = MeasureTextEx(app.font, dateText, UI_FONT_14, 0.4f).x;
    uiLabel(dateText, (Vector2){right - dateWidth, 37}, UI_FONT_14, UI_MUTED);
    float searchWidth = clampf((float)GetScreenWidth() * 0.22f, 180, 280);
    Rectangle searchBounds = {right - dateWidth - searchWidth - 28, 23, searchWidth, 40};
    uiTextInput(searchBounds, app.search, sizeof(app.search),
                "Rechercher...", false);
}

static void drawKpiCard(Rectangle bounds, const char *title, const char *value,
                        const char *caption, Color accent)
{
    uiPanel(bounds, UI_SURFACE);
    DrawCircle((int)(bounds.x + bounds.width - 26), (int)(bounds.y + 28), 5, accent);
    uiLabel(title, (Vector2){bounds.x + 18, bounds.y + 17}, UI_FONT_14, UI_MUTED);
    uiLabel(value, (Vector2){bounds.x + 18, bounds.y + 44}, UI_FONT_28, UI_TEXT);
    uiLabel(caption, (Vector2){bounds.x + 18, bounds.y + bounds.height - 25},
            UI_FONT_12, UI_MUTED);
}

static int selectedPeriodMonth(void)
{
    return app.allPeriods ? 0 : app.periodMonth;
}

static int selectedPeriodYear(void)
{
    return app.allPeriods ? 0 : app.periodYear;
}

static void drawDashboard(void)
{
    float x = SIDEBAR_WIDTH + 28.0f;
    float width = (float)GetScreenWidth() - x - 28;
    float gap = 16;
    float cardWidth = (width - gap * 3) / 4;
    float y = HEADER_HEIGHT + 10;
    int month = selectedPeriodMonth();
    int year = selectedPeriodYear();
    StatsResultat stats, annual;
    statsCalculer(month, year, &stats);
    statsCalculer(0, year, &annual);

    uiLabel(app.allPeriods ? "Indicateurs | Toutes les periodes" :
            TextFormat("Indicateurs | %s %d", monthNames[app.periodMonth - 1],
                       app.periodYear), (Vector2){x, y}, UI_FONT_16, UI_TEXT);
    float periodRight = (float)GetScreenWidth() - 28;
    if (uiButton((Rectangle){periodRight - 309, y - 8, 36, 36}, "<",
                 UI_BUTTON_SECONDARY, !app.allPeriods)) {
        if (--app.periodMonth < 1) {
            app.periodMonth = 12;
            app.periodYear--;
        }
    }
    uiButton((Rectangle){periodRight - 269, y - 8, 168, 36},
             TextFormat("%s %d", monthNames[app.periodMonth - 1], app.periodYear),
             UI_BUTTON_SECONDARY, !app.allPeriods);
    if (uiButton((Rectangle){periodRight - 97, y - 8, 36, 36}, ">",
                 UI_BUTTON_SECONDARY, !app.allPeriods)) {
        if (++app.periodMonth > 12) {
            app.periodMonth = 1;
            app.periodYear++;
        }
    }
    if (uiButton((Rectangle){periodRight - 57, y - 8, 57, 36}, "Tout",
                 app.allPeriods ? UI_BUTTON_PRIMARY : UI_BUTTON_SECONDARY, true)) {
        app.allPeriods = !app.allPeriods;
    }

    float cardY = y + 44;
    char value[64], caption[64];
    snprintf(value, sizeof(value), "%d", nb_salles);
    snprintf(caption, sizeof(caption), "%d espaces disponibles", nb_salles);
    drawKpiCard((Rectangle){x, cardY, cardWidth, 116}, "Salles", value,
                caption, UI_INDIGO);
    snprintf(value, sizeof(value), "%d", stats.reservations);
    snprintf(caption, sizeof(caption), "%d annulee(s)", stats.annulees);
    drawKpiCard((Rectangle){x + cardWidth + gap, cardY, cardWidth, 116},
                "Reservations actives", value, caption, UI_TEAL);
    moneyText(stats.chiffreAffaires, value, sizeof(value));
    snprintf(caption, sizeof(caption), "%s", app.allPeriods ?
             "Revenu cumule" : "Chiffre d'affaires");
    drawKpiCard((Rectangle){x + (cardWidth + gap) * 2, cardY, cardWidth, 116},
                "Revenu", TextFormat("%s TND", value), caption, UI_SUCCESS);
    float occupation = stats.heuresDisponibles > 0.0
        ? (float)(100.0 * stats.heuresReservees / stats.heuresDisponibles) : 0.0f;
    snprintf(value, sizeof(value), "%.1f %%", occupation);
    snprintf(caption, sizeof(caption), "%.0f h reservees",
             stats.heuresReservees);
    drawKpiCard((Rectangle){x + (cardWidth + gap) * 3, cardY, cardWidth, 116},
                "Taux d'occupation", value, caption, UI_WARNING);

    float rowY = cardY + 134;
    float chartWidth = width * 0.58f;
    float lowerHeight = clampf((float)GetScreenHeight() - rowY - 28, 195, 280);
    Rectangle chart = {x, rowY, chartWidth, lowerHeight};
    uiPanel(chart, UI_SURFACE);
    uiLabel("Activite mensuelle", (Vector2){chart.x + 18, chart.y + 16},
            UI_FONT_16, UI_TEXT);
    uiLabel(app.allPeriods ? "Toutes les annees" : TextFormat("Annee %d", app.periodYear),
            (Vector2){chart.x + chart.width - 145, chart.y + 19},
            UI_FONT_12, UI_MUTED);

    int maximum = 1;
    for (int i = 0; i < 12; i++) {
        if (annual.reservationsParMois[i] > maximum) maximum = annual.reservationsParMois[i];
    }
    float graphTop = chart.y + 61;
    float graphHeight = chart.height - 104;
    float barWidth = (chart.width - 48) / 12.0f;
    static const char *shortMonths[] = {"J","F","M","A","M","J","J","A","S","O","N","D"};
    for (int i = 0; i < 12; i++) {
        float barH = graphHeight * annual.reservationsParMois[i] / maximum;
        float barX = chart.x + 22 + i * barWidth;
        float barY = graphTop + graphHeight - barH;
        DrawRectangleRounded((Rectangle){barX + 6, graphTop, barWidth - 12, graphHeight},
                             0.3f, 5, ColorAlpha(UI_BORDER, 0.35f));
        if (barH > 1) {
            DrawRectangleRounded((Rectangle){barX + 6, barY, barWidth - 12, barH},
                                 0.3f, 5, i == app.periodMonth - 1 && !app.allPeriods ?
                                 UI_TEAL : ColorAlpha(UI_INDIGO, 0.9f));
        }
        uiLabel(shortMonths[i], (Vector2){barX + barWidth * 0.5f - 4, graphTop + graphHeight + 9},
                UI_FONT_12, UI_MUTED);
    }

    float sideX = chart.x + chart.width + 16;
    float sideWidth = width - chart.width - 16;
    Rectangle popular = {sideX, rowY, sideWidth, lowerHeight};
    uiPanel(popular, UI_SURFACE);
    uiLabel("Espaces les plus demandes", (Vector2){popular.x + 18, popular.y + 16},
            UI_FONT_16, UI_TEXT);
    int topIndices[3] = {-1, -1, -1};
    for (int rank = 0; rank < 3; rank++) {
        int topCount = 0, topIndex = -1;
        for (int i = 0; i < nb_salles; i++) {
            bool used = false;
            for (int previous = 0; previous < rank; previous++) {
                if (topIndices[previous] == i) used = true;
            }
            if (!used && stats.reservationsParSalle[i] > topCount) {
                topCount = stats.reservationsParSalle[i];
                topIndex = i;
            }
        }
        topIndices[rank] = topIndex;
        float lineY = popular.y + 58 + rank * 48.0f;
        if (topIndex >= 0 && topCount > 0) {
            char count[40];
            snprintf(count, sizeof(count), "%d reservation(s)", topCount);
            DrawCircle((int)(popular.x + 28), (int)(lineY + 12), 12,
                       rank == 0 ? UI_TEAL : ColorAlpha(UI_INDIGO, 0.7f));
            uiLabel(TextFormat("%d", rank + 1), (Vector2){popular.x + 24, lineY + 5},
                    UI_FONT_12, UI_WHITE);
            uiLabel(salles[topIndex].nom, (Vector2){popular.x + 50, lineY},
                    UI_FONT_14, UI_TEXT);
            uiLabel(count, (Vector2){popular.x + 50, lineY + 19},
                    UI_FONT_12, UI_MUTED);
        } else {
            uiLabel("--", (Vector2){popular.x + 20, lineY}, UI_FONT_14, UI_MUTED);
            uiLabel("Aucune reservation", (Vector2){popular.x + 50, lineY},
                    UI_FONT_14, UI_MUTED);
        }
    }
    DrawLineEx((Vector2){popular.x + 18, popular.y + popular.height - 46},
               (Vector2){popular.x + popular.width - 18, popular.y + popular.height - 46},
               1, UI_BORDER);
    uiLabel("Heures reservees", (Vector2){popular.x + 20, popular.y + popular.height - 34},
            UI_FONT_12, UI_MUTED);
    uiLabel(TextFormat("%.1f h", stats.heuresReservees),
            (Vector2){popular.x + popular.width - 82, popular.y + popular.height - 36},
            UI_FONT_14, UI_TEAL);

    float bottomY = rowY + lowerHeight + 15;
    if (bottomY + 40 < GetScreenHeight()) {
        uiLabel("Dernieres reservations", (Vector2){x + 2, bottomY},
                UI_FONT_16, UI_TEXT);
        int shown = 0;
        for (int i = nb_reservations - 1; i >= 0 && shown < 3; i--) {
            if (strcmp(reservations[i].statut, "annulee") == 0) continue;
            float rowX = x + 210 + shown * (width - 220) / 3.0f;
            uiLabel(reservations[i].nom_client, (Vector2){rowX, bottomY},
                    UI_FONT_12, UI_MUTED);
            uiLabel(TextFormat("%s | %s", reservations[i].salle.nom,
                               reservations[i].date),
                    (Vector2){rowX, bottomY + 18}, UI_FONT_12, UI_TEXT);
            shown++;
        }
    }
}

static void roomEquipment(bool wifi, bool projector, bool whiteboard,
                          float x, float y)
{
    float nextX = x;
    const char *labels[] = {"Wi-Fi", "Projecteur", "Tableau blanc"};
    bool enabled[] = {wifi, projector, whiteboard};
    for (int i = 0; i < 3; i++) {
        if (!enabled[i]) continue;
        float textWidth = MeasureTextEx(app.font, labels[i], UI_FONT_12, 0.4f).x + 20;
        Rectangle badge = {nextX, y, textWidth, 25};
        drawRounded(badge, ColorAlpha(UI_TEAL, 0.14f));
        uiLabel(labels[i], (Vector2){badge.x + 10, badge.y + 6}, UI_FONT_12, UI_TEAL);
        nextX += textWidth + 7;
    }
    if (!wifi && !projector && !whiteboard) {
        uiLabel("Aucun equipement indique", (Vector2){x, y + 4}, UI_FONT_12, UI_MUTED);
    }
}

static void parseRoomEquipment(const char *equipment)
{
    app.roomWifi = strstr(equipment, "Wi-Fi") != NULL;
    app.roomProjector = strstr(equipment, "Projecteur") != NULL;
    app.roomWhiteboard = strstr(equipment, "Tableau blanc") != NULL;
}

static void beginRoomForm(int index)
{
    app.modal = MODAL_ROOM_FORM;
    app.editingRoom = index;
    app.roomName[0] = '\0';
    app.roomCapacityText[0] = '\0';
    app.roomRateText[0] = '\0';
    app.roomWifi = app.roomProjector = app.roomWhiteboard = false;
    if (index >= 0 && index < nb_salles) {
        Salle *room = &salles[index];
        snprintf(app.roomName, sizeof(app.roomName), "%s", room->nom);
        snprintf(app.roomCapacityText, sizeof(app.roomCapacityText), "%d", room->capacite);
        moneyText(room->tarif_horaire, app.roomRateText, sizeof(app.roomRateText));
        parseRoomEquipment(room->equipements);
    }
}

static void drawRoomCard(Rectangle bounds, int index)
{
    Salle *room = &salles[index];
    uiPanel(bounds, UI_SURFACE);
    Rectangle iconBox = {bounds.x + 18, bounds.y + 18, 44, 44};
    drawRounded(iconBox, ColorAlpha(UI_INDIGO, 0.24f));
    drawIcon(PAGE_ROOMS, iconBox.x + 12, iconBox.y + 12, UI_TEAL);
    uiLabel(room->nom, (Vector2){bounds.x + 74, bounds.y + 22}, UI_FONT_20, UI_TEXT);
    uiLabel(TextFormat("%d places", room->capacite),
            (Vector2){bounds.x + 74, bounds.y + 49}, UI_FONT_12, UI_MUTED);

    char price[48];
    moneyText(room->tarif_horaire, price, sizeof(price));
    uiLabel(TextFormat("%s TND", price), (Vector2){bounds.x + 18, bounds.y + 84},
            UI_FONT_28, UI_TEXT);
    uiLabel("par heure", (Vector2){bounds.x + 18, bounds.y + 117}, UI_FONT_12, UI_MUTED);
    roomEquipment(strstr(room->equipements, "Wi-Fi") != NULL,
                  strstr(room->equipements, "Projecteur") != NULL,
                  strstr(room->equipements, "Tableau blanc") != NULL,
                  bounds.x + 18, bounds.y + 146);

    SallesDisponibles availableRooms;
    bool available = sallesDisponibles(1, app.plannerDate, "09:00", "17:00",
                                      &availableRooms) == RES_OK;
    if (available) {
        available = false;
        for (size_t i = 0; i < availableRooms.nombre; i++) {
            if (strcmp(availableRooms.elements[i].nom, room->nom) == 0) {
                available = true;
                break;
            }
        }
    }
    DrawCircle((int)(bounds.x + 25), (int)(bounds.y + bounds.height - 21),
               4, available ? UI_SUCCESS : UI_WARNING);
    uiLabel(available ? "Libre" : "Occupee",
            (Vector2){bounds.x + 37, bounds.y + bounds.height - 29},
            UI_FONT_12, available ? UI_SUCCESS : UI_WARNING);
    Rectangle deleteButton = {bounds.x + bounds.width - 92,
                              bounds.y + bounds.height - 40, 74, 30};
    Rectangle editButton = {bounds.x + bounds.width - 188,
                            bounds.y + bounds.height - 40, 82, 30};
    if (uiButton(editButton, "Modifier", UI_BUTTON_GHOST, true)) beginRoomForm(index);
    if (uiButton(deleteButton, "Supprimer", UI_BUTTON_GHOST, true)) {
        app.selectedRoom = index;
        app.modal = MODAL_DELETE_ROOM;
    }
}

static void drawRooms(void)
{
    float x = SIDEBAR_WIDTH + 28.0f;
    float width = (float)GetScreenWidth() - x - 28;
    uiLabel("Des espaces adaptes a chaque rencontre.",
            (Vector2){x, HEADER_HEIGHT + 8}, UI_FONT_14, UI_MUTED);
    Rectangle addButton = {GetScreenWidth() - 216.0f, HEADER_HEIGHT + 2, 188, 40};
    if (uiButton(addButton, "+  Ajouter une salle", UI_BUTTON_PRIMARY, true)) {
        beginRoomForm(-1);
    }

    int columns = width >= 980 ? 3 : 2;
    float gap = 16;
    float cardW = (width - gap * (columns - 1)) / columns;
    float cardH = 218;
    float y = HEADER_HEIGHT + 62;
    for (int i = app.roomScroll; i < nb_salles; i++) {
        int visibleIndex = i - app.roomScroll;
        Rectangle card = {x + (visibleIndex % columns) * (cardW + gap),
                          y + (visibleIndex / columns) * (cardH + gap),
                          cardW, cardH};
        drawRoomCard(card, i);
        if (card.y + card.height > GetScreenHeight() - 20) break;
    }
    if (nb_salles > columns * 2) {
        if (CheckCollisionPointRec(GetMousePosition(),
                (Rectangle){x, y, width, GetScreenHeight() - y}) &&
            GetMouseWheelMove() != 0) {
            app.roomScroll -= (int)GetMouseWheelMove();
            if (app.roomScroll < 0) app.roomScroll = 0;
            if (app.roomScroll > nb_salles - 1) app.roomScroll = nb_salles - 1;
        }
        Rectangle scrollTrack = {GetScreenWidth() - 34.0f, y, 5,
                                 GetScreenHeight() - y - 24};
        if (scrollTrack.height > 0) {
            DrawRectangleRounded(scrollTrack, 0.8f, 4, UI_BORDER);
            float thumbH = fmaxf(34, scrollTrack.height * 2 / nb_salles);
            float thumbY = scrollTrack.y +
                (scrollTrack.height - thumbH) * app.roomScroll / fmaxf(1, nb_salles - 1);
            DrawRectangleRounded((Rectangle){scrollTrack.x, thumbY, 5, thumbH},
                                 0.8f, 4, UI_TEAL);
        }
    }
}

static void drawWizardProgress(float x, float y, float width)
{
    const char *steps[] = {"Client", "Date et creneau", "Salle", "Confirmation"};
    float gap = 14;
    float stepW = (width - 3 * gap) / 4;
    for (int i = 0; i < 4; i++) {
        float sx = x + i * (stepW + gap);
        DrawRectangleRounded((Rectangle){sx, y, stepW, 4}, 0.8f, 4,
                             i <= app.wizardStep ? UI_INDIGO : UI_BORDER);
        DrawCircle((int)(sx + 10), (int)(y + 26), 10,
                   i == app.wizardStep ? UI_TEAL :
                   i < app.wizardStep ? UI_INDIGO : UI_SURFACE_RAISED);
        uiLabel(TextFormat("%d", i + 1), (Vector2){sx + 6, y + 19},
                UI_FONT_12, UI_TEXT);
        uiLabel(steps[i], (Vector2){sx + 27, y + 20}, UI_FONT_12,
                i <= app.wizardStep ? UI_TEXT : UI_MUTED);
    }
}

static ResCode getBookingRooms(SallesDisponibles *available, int *people)
{
    if (app.bookingClient[0] == '\0' ||
        !parsePositiveInt(app.bookingPeople, people)) return RES_ARGUMENT_INVALIDE;
    return sallesDisponibles(*people, app.bookingDate, TextFormat("%02d:%02d",
                             app.startMinutes / 60, app.startMinutes % 60),
                             TextFormat("%02d:%02d", app.endMinutes / 60,
                                        app.endMinutes % 60), available);
}

static void nextWizardStep(void)
{
    if (app.wizardStep == 0) {
        int people;
        if (app.bookingClient[0] == '\0' ||
            !parsePositiveInt(app.bookingPeople, &people)) {
            notify("Saisissez le nom du client et un nombre de personnes valide.", UI_ERROR);
            return;
        }
    } else if (app.wizardStep == 1) {
        SallesDisponibles available;
        int people;
        ResCode code = parsePositiveInt(app.bookingPeople, &people)
            ? sallesDisponibles(people, app.bookingDate,
                TextFormat("%02d:%02d", app.startMinutes / 60, app.startMinutes % 60),
                TextFormat("%02d:%02d", app.endMinutes / 60, app.endMinutes % 60),
                &available)
            : RES_NOMBRE_INVALIDE;
        if (code != RES_OK) {
            notify(resCodeMessage(code), UI_ERROR);
            return;
        }
        app.selectedWizardRoom = -1;
    } else if (app.wizardStep == 2 && app.selectedWizardRoom < 0) {
        notify("Selectionnez une salle recommandee.", UI_ERROR);
        return;
    } else if (app.wizardStep == 3) {
        Salle *room = &salles[app.selectedWizardRoom];
        Reservation result;
        ResCode code;
        if (app.editingReservation > 0) {
            code = resModifier(app.editingReservation, app.bookingClient,
                app.personCount, app.bookingDate,
                TextFormat("%02d:%02d", app.startMinutes / 60, app.startMinutes % 60),
                TextFormat("%02d:%02d", app.endMinutes / 60, app.endMinutes % 60),
                room->nom, &result);
        } else {
            code = resCreer(app.bookingClient, app.personCount, app.bookingDate,
                TextFormat("%02d:%02d", app.startMinutes / 60, app.startMinutes % 60),
                TextFormat("%02d:%02d", app.endMinutes / 60, app.endMinutes % 60),
                room->nom, &result);
        }
        notify(code == RES_OK ?
            (app.editingReservation > 0 ? "Reservation modifiee avec succes." :
                                         "Reservation confirmee et facture creee.") :
            resCodeMessage(code), code == RES_OK ? UI_SUCCESS : UI_ERROR);
        if (code == RES_OK) {
            app.selectedReservation = result.id;
            app.selectedInvoice = result.id;
            app.page = PAGE_RESERVATIONS;
            app.editingReservation = -1;
        }
        return;
    }
    app.wizardStep++;
}

static void drawBooking(void)
{
    float x = SIDEBAR_WIDTH + 28.0f;
    float width = (float)GetScreenWidth() - x - 28;
    float top = HEADER_HEIGHT + 8;
    Rectangle form = {x, top, width, GetScreenHeight() - top - 24};
    uiPanel(form, UI_SURFACE);
    drawWizardProgress(x + 28, top + 28, width - 56);
    float startY = top + 98;
    uiLabel(app.editingReservation > 0 ? "Modifier une reservation" :
            "Creer une reservation", (Vector2){x + 28, startY}, UI_FONT_20, UI_TEXT);

    if (app.wizardStep == 0) {
        uiLabel("Commencez par les informations de votre client.",
                (Vector2){x + 28, startY + 34}, UI_FONT_14, UI_MUTED);
        uiLabel("Nom du client", (Vector2){x + 28, startY + 82}, UI_FONT_14, UI_TEXT);
        uiTextInput((Rectangle){x + 28, startY + 108, fminf(540, width - 56), 44},
                    app.bookingClient, sizeof(app.bookingClient),
                    "Ex. Entreprise ou nom du responsable", false);
        uiLabel("Nombre de participants", (Vector2){x + 28, startY + 180},
                UI_FONT_14, UI_TEXT);
        uiTextInput((Rectangle){x + 28, startY + 206, 210, 44},
                    app.bookingPeople, sizeof(app.bookingPeople), "Ex. 12", true);
        uiLabel("Nous recommanderons les salles adaptees a votre groupe.",
                (Vector2){x + 28, startY + 272}, UI_FONT_12, UI_MUTED);
    } else if (app.wizardStep == 1) {
        uiLabel("Choisissez le jour et la plage horaire souhaitee.",
                (Vector2){x + 28, startY + 35}, UI_FONT_14, UI_MUTED);
        uiLabel("Date", (Vector2){x + 28, startY + 84}, UI_FONT_14, UI_TEXT);
        uiDatePicker((Rectangle){x + 28, startY + 111, 250, 44},
                     app.bookingDate, sizeof(app.bookingDate));
        uiLabel("Heure de debut", (Vector2){x + 28, startY + 190},
                UI_FONT_14, UI_TEXT);
        uiTimePicker((Rectangle){x + 28, startY + 218, 132, 42}, &app.startMinutes);
        uiLabel("Heure de fin", (Vector2){x + 205, startY + 190},
                UI_FONT_14, UI_TEXT);
        uiTimePicker((Rectangle){x + 205, startY + 218, 132, 42}, &app.endMinutes);
        uiLabel("Horaires d'ouverture : 08:00 a 23:59. La fin doit suivre le debut.",
                (Vector2){x + 28, startY + 288}, UI_FONT_12, UI_MUTED);
    } else if (app.wizardStep == 2) {
        int people;
        SallesDisponibles available;
        ResCode code = getBookingRooms(&available, &people);
        if (code != RES_OK) {
            uiLabel(resCodeMessage(code), (Vector2){x + 28, startY + 48},
                    UI_FONT_14, UI_ERROR);
        } else {
            uiLabel(TextFormat("%zu salle(s) disponible(s) pour %d personne(s)",
                    available.nombre, people),
                    (Vector2){x + 28, startY + 36}, UI_FONT_14, UI_MUTED);
            int columns = width > 930 ? 3 : 2;
            float gap = 12;
            float cardW = (width - 56 - gap * (columns - 1)) / columns;
            for (size_t i = 0; i < available.nombre; i++) {
                float cardY = startY + 76 + (i / columns) * 142;
                Rectangle card = {x + 28 + (i % columns) * (cardW + gap),
                                  cardY, cardW, 128};
                bool selected = app.selectedWizardRoom >= 0 &&
                    strcmp(salles[app.selectedWizardRoom].nom,
                           available.elements[i].nom) == 0;
                uiPanel(card, selected ? ColorAlpha(UI_INDIGO, 0.24f) : UI_SURFACE_RAISED);
                uiLabel(available.elements[i].nom,
                        (Vector2){card.x + 16, card.y + 14}, UI_FONT_20, UI_TEXT);
                char rate[40];
                moneyText(available.elements[i].tarif_horaire, rate, sizeof(rate));
                uiLabel(TextFormat("%d places | %s TND/h",
                        available.elements[i].capacite, rate),
                        (Vector2){card.x + 16, card.y + 45}, UI_FONT_12, UI_MUTED);
                roomEquipment(strstr(available.elements[i].equipements, "Wi-Fi") != NULL,
                    strstr(available.elements[i].equipements, "Projecteur") != NULL,
                    strstr(available.elements[i].equipements, "Tableau blanc") != NULL,
                    card.x + 16, card.y + 75);
                if (uiButton(card, "", selected ? UI_BUTTON_PRIMARY : UI_BUTTON_GHOST, true)) {
                    for (int roomIndex = 0; roomIndex < nb_salles; roomIndex++) {
                        if (strcmp(salles[roomIndex].nom,
                                   available.elements[i].nom) == 0) {
                            app.selectedWizardRoom = roomIndex;
                            app.personCount = people;
                            break;
                        }
                    }
                }
            }
        }
    } else {
        if (app.selectedWizardRoom < 0 || app.selectedWizardRoom >= nb_salles) {
            app.wizardStep = 2;
        } else {
            Salle *room = &salles[app.selectedWizardRoom];
            int people = 0;
            parsePositiveInt(app.bookingPeople, &people);
            int duration = app.endMinutes - app.startMinutes;
            float amount = (duration / 60.0f) * room->tarif_horaire;
            uiLabel("Relisez les details avant de confirmer.",
                    (Vector2){x + 28, startY + 34}, UI_FONT_14, UI_MUTED);
            const char *labels[] = {"Client", "Salle", "Date", "Creneau",
                                    "Participants", "Duree", "Estimation"};
            char values[7][100];
            snprintf(values[0], sizeof(values[0]), "%s", app.bookingClient);
            snprintf(values[1], sizeof(values[1]), "%s", room->nom);
            snprintf(values[2], sizeof(values[2]), "%s", app.bookingDate);
            snprintf(values[3], sizeof(values[3]), "%02d:%02d - %02d:%02d",
                     app.startMinutes / 60, app.startMinutes % 60,
                     app.endMinutes / 60, app.endMinutes % 60);
            snprintf(values[4], sizeof(values[4]), "%d personne(s)", people);
            snprintf(values[5], sizeof(values[5]), "%d h %02d min",
                     duration / 60, duration % 60);
            char amountText[48];
            moneyText(amount, amountText, sizeof(amountText));
            snprintf(values[6], sizeof(values[6]), "%s TND", amountText);
            for (int i = 0; i < 7; i++) {
                float lineY = startY + 82 + i * 46;
                uiLabel(labels[i], (Vector2){x + 34, lineY}, UI_FONT_14, UI_MUTED);
                uiLabel(values[i], (Vector2){x + 240, lineY}, UI_FONT_14,
                        i == 6 ? UI_TEAL : UI_TEXT);
                DrawLineEx((Vector2){x + 30, lineY + 29},
                           (Vector2){x + width - 30, lineY + 29}, 1, UI_BORDER);
            }
        }
    }

    if (app.wizardStep > 0 && uiButton((Rectangle){x + 28, form.y + form.height - 62,
            132, 42}, "Retour", UI_BUTTON_SECONDARY, true)) {
        app.wizardStep--;
    }
    const char *nextLabel = app.wizardStep == 3 ? "Confirmer la reservation" : "Continuer";
    float nextWidth = app.wizardStep == 3 ? 230 : 150;
    if (uiButton((Rectangle){x + width - nextWidth - 28,
            form.y + form.height - 62, nextWidth, 42},
            nextLabel, UI_BUTTON_PRIMARY, true)) {
        nextWizardStep();
    }
}

static int reservationMatches(const Reservation *reservation)
{
    if (app.statusFilter == 1 && strcmp(reservation->statut, "confirmee") != 0) return 0;
    if (app.statusFilter == 2 && strcmp(reservation->statut, "modifiee") != 0) return 0;
    if (app.statusFilter == 3 && strcmp(reservation->statut, "annulee") != 0) return 0;
    if (app.roomFilter > 0 && app.roomFilter <= nb_salles &&
        strcmp(reservation->salle.nom, salles[app.roomFilter - 1].nom) != 0) return 0;
    if (app.reservationDateFilter[0] != '\0' &&
        strcmp(reservation->date, app.reservationDateFilter) != 0) return 0;
    return containsText(reservation->nom_client, app.search) ||
           containsText(reservation->salle.nom, app.search) ||
           containsText(reservation->date, app.search);
}

static void drawReservationDetails(float x, float y, float width, float height)
{
    Reservation *selected = resTrouver(app.selectedReservation);
    Rectangle panel = {x, y, width, height};
    uiPanel(panel, UI_SURFACE);
    uiLabel("Details", (Vector2){x + 18, y + 16}, UI_FONT_16, UI_TEXT);
    if (selected == NULL) {
        drawTextWrapped("Selectionnez une reservation pour afficher ses details.",
                        x + 18, y + 58, width - 36, UI_FONT_14, UI_MUTED);
        return;
    }
    char amount[40];
    moneyText(selected->tarif, amount, sizeof(amount));
    uiLabel(TextFormat("#%d", selected->id), (Vector2){x + 18, y + 54},
            UI_FONT_28, UI_TEAL);
    const char *labels[] = {"Client", "Salle", "Date", "Horaire",
                            "Participants", "Statut", "Montant"};
    char values[7][96];
    snprintf(values[0], sizeof(values[0]), "%s", selected->nom_client);
    snprintf(values[1], sizeof(values[1]), "%s", selected->salle.nom);
    snprintf(values[2], sizeof(values[2]), "%s", selected->date);
    snprintf(values[3], sizeof(values[3]), "%s - %s",
             selected->heure_debut, selected->heure_fin);
    snprintf(values[4], sizeof(values[4]), "%d", selected->nombre_personnes);
    snprintf(values[5], sizeof(values[5]), "%s", selected->statut);
    snprintf(values[6], sizeof(values[6]), "%s TND", amount);
    for (int i = 0; i < 7; i++) {
        float rowY = y + 96 + i * 46;
        uiLabel(labels[i], (Vector2){x + 18, rowY}, UI_FONT_12, UI_MUTED);
        drawTextWrapped(values[i], x + 18, rowY + 17, width - 36,
                        UI_FONT_14, i == 6 ? UI_TEAL : UI_TEXT);
    }
    float buttonY = y + height - 126;
    if (strcmp(selected->statut, "annulee") != 0 &&
        uiButton((Rectangle){x + 18, buttonY, width - 36, 38},
                 "Modifier", UI_BUTTON_SECONDARY, true)) {
        startBooking(selected->id);
    }
    if (strcmp(selected->statut, "annulee") != 0 &&
        uiButton((Rectangle){x + 18, buttonY + 47, (width - 44) / 2, 38},
                 "Annuler", UI_BUTTON_SECONDARY, true)) {
        app.modal = MODAL_RESERVATION_ACTION;
        app.action = ACTION_CANCEL_RESERVATION;
    }
    if (uiButton((Rectangle){x + 26 + (width - 44) / 2, buttonY + 47,
            (width - 44) / 2, 38}, "Supprimer", UI_BUTTON_DANGER, true)) {
        app.modal = MODAL_RESERVATION_ACTION;
        app.action = ACTION_DELETE_RESERVATION;
    }
}

static void drawReservations(void)
{
    float x = SIDEBAR_WIDTH + 28.0f;
    float width = (float)GetScreenWidth() - x - 28;
    float top = HEADER_HEIGHT + 8;
    float detailWidth = clampf(width * 0.29f, 278, 340);
    float gap = 16;
    float tableWidth = width - detailWidth - gap;

    uiLabel(TextFormat("%d enregistrement(s)", nb_reservations),
            (Vector2){x, top}, UI_FONT_14, UI_MUTED);

    float panelY = top + 76;
    float panelH = GetScreenHeight() - panelY - 24;
    Rectangle table = {x, panelY, tableWidth, panelH};
    uiPanel(table, UI_SURFACE);
    float headerY = panelY + 14;
    float colId = x + 16, colClient = x + 76, colRoom = x + tableWidth * 0.49f;
    float colDate = x + tableWidth * 0.68f, colAmount = x + tableWidth - 105;
    uiLabel("ID", (Vector2){colId, headerY}, UI_FONT_12, UI_MUTED);
    uiLabel("CLIENT", (Vector2){colClient, headerY}, UI_FONT_12, UI_MUTED);
    uiLabel("SALLE", (Vector2){colRoom, headerY}, UI_FONT_12, UI_MUTED);
    uiLabel("DATE", (Vector2){colDate, headerY}, UI_FONT_12, UI_MUTED);
    uiLabel("MONTANT", (Vector2){colAmount, headerY}, UI_FONT_12, UI_MUTED);
    DrawLineEx((Vector2){x + 14, headerY + 24},
               (Vector2){x + tableWidth - 14, headerY + 24}, 1, UI_BORDER);

    int order[MAX_VISIBLE_ROWS], count = 0;
    for (int i = 0; i < nb_reservations && count < MAX_VISIBLE_ROWS; i++) {
        if (reservationMatches(&reservations[i])) order[count++] = i;
    }
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            int left = order[i], right = order[j];
            int comparison = app.sortDescending
                ? compareDate(reservations[left].date, reservations[right].date)
                : compareDate(reservations[right].date, reservations[left].date);
            if (comparison > 0) {
                int temporary = order[i];
                order[i] = order[j];
                order[j] = temporary;
            }
        }
    }
    float rowHeight = 49;
    int visibleRows = (int)((panelH - 57) / rowHeight);
    if (visibleRows < 1) visibleRows = 1;
    int maxScroll = count > visibleRows ? count - visibleRows : 0;
    if (app.reservationScroll > maxScroll) app.reservationScroll = maxScroll;
    if (inside((Rectangle){x, panelY + 42, tableWidth, panelH - 42})) {
        float wheel = GetMouseWheelMove();
        if (wheel < 0 && app.reservationScroll < maxScroll) app.reservationScroll++;
        if (wheel > 0 && app.reservationScroll > 0) app.reservationScroll--;
    }

    int row = 0;
    for (int i = app.reservationScroll; i < count && row < visibleRows; i++, row++) {
        Reservation *reservation = &reservations[order[i]];
        float rowY = headerY + 36 + row * rowHeight;
        Rectangle rowBounds = {x + 8, rowY - 5, tableWidth - 16, 42};
        bool selected = reservation->id == app.selectedReservation;
        bool hovered = CheckCollisionPointRec(GetMousePosition(), rowBounds);
        if (selected) drawRounded(rowBounds, ColorAlpha(UI_INDIGO, 0.2f));
        else if (hovered) drawRounded(rowBounds, ColorAlpha(UI_SURFACE_RAISED, 0.7f));
        Color statusColor = strcmp(reservation->statut, "annulee") == 0 ? UI_ERROR :
            strcmp(reservation->statut, "modifiee") == 0 ? UI_WARNING : UI_SUCCESS;
        uiLabel(TextFormat("#%d", reservation->id), (Vector2){colId, rowY + 5},
                UI_FONT_12, UI_TEAL);
        uiLabel(reservation->nom_client, (Vector2){colClient, rowY + 5},
                UI_FONT_12, UI_TEXT);
        uiLabel(reservation->salle.nom, (Vector2){colRoom, rowY + 5},
                UI_FONT_12, UI_TEXT);
        uiLabel(reservation->date, (Vector2){colDate, rowY + 5},
                UI_FONT_12, UI_MUTED);
        char amount[32];
        moneyText(reservation->tarif, amount, sizeof(amount));
        uiLabel(TextFormat("%s TND", amount), (Vector2){colAmount, rowY + 5},
                UI_FONT_12, UI_TEXT);
        DrawCircle((int)(colDate - 9), (int)(rowY + 11), 3, statusColor);
        if (hovered) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                app.selectedReservation = reservation->id;
                app.selectedInvoice = reservation->id;
            }
        }
        if (row < visibleRows - 1) {
            DrawLineEx((Vector2){x + 18, rowY + 37},
                       (Vector2){x + tableWidth - 18, rowY + 37}, 1,
                       ColorAlpha(UI_BORDER, 0.6f));
        }
    }
    if (count == 0) {
        uiLabel("Aucun resultat pour ces filtres.",
                (Vector2){x + 24, headerY + 54}, UI_FONT_14, UI_MUTED);
    }
    Rectangle dateSort = {colDate - 4, headerY - 6, 62, 28};
    if (inside(dateSort) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        app.sortDescending = !app.sortDescending;
    }

    drawReservationDetails(x + tableWidth + gap, panelY, detailWidth, panelH);

    const char *statusOptions[] = {"Tous les statuts", "Confirmee",
                                   "Modifiee", "Annulee"};
    uiDropdown((Rectangle){x, top + 29, 150, 36}, statusOptions,
               4, &app.statusFilter);
    const char *roomOptions[MAX_SALLES + 1];
    roomOptions[0] = "Toutes les salles";
    for (int i = 0; i < nb_salles; i++) roomOptions[i + 1] = salles[i].nom;
    uiDropdown((Rectangle){x + 158, top + 29, 170, 36}, roomOptions,
               (size_t)nb_salles + 1, &app.roomFilter);
    uiDatePicker((Rectangle){x + 336, top + 29, 220, 36},
                 app.reservationDateFilter, sizeof(app.reservationDateFilter));
    if (uiButton((Rectangle){x + 562, top + 29, 64, 36}, "Effacer",
                 UI_BUTTON_GHOST, true)) {
        app.reservationDateFilter[0] = '\0';
    }
    if (uiButton((Rectangle){x + width - 184, top + 29, 184, 36},
                 "+ Nouvelle reservation", UI_BUTTON_PRIMARY, true)) {
        startBooking(-1);
    }
}

static void drawPlanning(void)
{
    float x = SIDEBAR_WIDTH + 28.0f;
    float width = (float)GetScreenWidth() - x - 28;
    float top = HEADER_HEIGHT + 8;
    uiLabel("Vue journee", (Vector2){x, top}, UI_FONT_16, UI_TEXT);
    if (uiButton((Rectangle){x + 88, top - 8, 36, 42}, "<",
                 UI_BUTTON_SECONDARY, true)) {
        shiftDate(app.plannerDate, -1);
    }
    uiDatePicker((Rectangle){x + 130, top - 8, 250, 42},
                 app.plannerDate, sizeof(app.plannerDate));
    if (uiButton((Rectangle){x + 386, top - 8, 36, 42}, ">",
                 UI_BUTTON_SECONDARY, true)) {
        shiftDate(app.plannerDate, 1);
    }
    uiLabel("Faites defiler le planning pour parcourir les salles.",
            (Vector2){x + 436, top + 4}, UI_FONT_12, UI_MUTED);

    float y = top + 55;
    float height = GetScreenHeight() - y - 22;
    Rectangle panel = {x, y, width, height};
    uiPanel(panel, UI_SURFACE);
    float labelW = 126;
    float gridX = x + labelW + 20;
    float gridW = width - labelW - 36;
    int roomRows = (int)((height - 54) / 59);
    if (roomRows < 1) roomRows = 1;
    int maxScroll = nb_salles > roomRows ? nb_salles - roomRows : 0;
    if (app.plannerScroll > maxScroll) app.plannerScroll = maxScroll;
    if (inside(panel)) {
        float wheel = GetMouseWheelMove();
        if (wheel < 0 && app.plannerScroll < maxScroll) app.plannerScroll++;
        if (wheel > 0 && app.plannerScroll > 0) app.plannerScroll--;
    }

    int startDay, month, year;
    dateParts(app.plannerDate, &startDay, &month, &year);
    for (int hour = 8; hour <= 23; hour += 2) {
        float hx = gridX + (hour - 8) / 16.0f * gridW;
        uiLabel(TextFormat("%02d:00", hour), (Vector2){hx - 18, y + 16},
                UI_FONT_12, UI_MUTED);
    }
    DrawLineEx((Vector2){gridX, y + 43}, (Vector2){gridX + gridW, y + 43},
               1, UI_BORDER);

    int shown = 0;
    for (int i = app.plannerScroll; i < nb_salles && shown < roomRows; i++, shown++) {
        float rowY = y + 53 + shown * 59.0f;
        uiLabel(salles[i].nom, (Vector2){x + 18, rowY + 14}, UI_FONT_14, UI_TEXT);
        DrawLineEx((Vector2){gridX, rowY + 48}, (Vector2){gridX + gridW, rowY + 48},
                   1, ColorAlpha(UI_BORDER, 0.5f));
        for (int hour = 8; hour <= 24; hour++) {
            float hx = gridX + (hour - 8) / 16.0f * gridW;
            DrawLineEx((Vector2){hx, rowY + 2}, (Vector2){hx, rowY + 47},
                       1, ColorAlpha(UI_BORDER, 0.42f));
        }
        for (int j = 0; j < nb_reservations; j++) {
            Reservation *reservation = &reservations[j];
            if (strcmp(reservation->statut, "annulee") == 0 ||
                strcmp(reservation->salle.nom, salles[i].nom) != 0 ||
                strcmp(reservation->date, app.plannerDate) != 0) continue;
            int begin = heureEnMinutes(reservation->heure_debut);
            int end = heureEnMinutes(reservation->heure_fin);
            float blockX = gridX + (begin - 480) / 960.0f * gridW;
            float blockW = (end - begin) / 960.0f * gridW;
            Rectangle block = {blockX + 1, rowY + 7, fmaxf(12, blockW - 2), 34};
            drawRounded(block, ColorAlpha(UI_INDIGO, 0.9f));
            if (block.width > 100) {
                uiLabel(TextFormat("%s | %s-%s", reservation->nom_client,
                        reservation->heure_debut, reservation->heure_fin),
                        (Vector2){block.x + 8, block.y + 10}, UI_FONT_12, UI_WHITE);
            } else if (block.width > 54) {
                uiLabel(TextFormat("%s-%s", reservation->heure_debut,
                        reservation->heure_fin),
                        (Vector2){block.x + 5, block.y + 10}, UI_FONT_12, UI_WHITE);
            }
        }
    }
    (void)startDay;
    (void)month;
    (void)year;
}

static void drawInvoiceList(float x, float y, float width, float height)
{
    Rectangle panel = {x, y, width, height};
    uiPanel(panel, UI_SURFACE);
    uiLabel("Factures", (Vector2){x + 16, y + 16}, UI_FONT_16, UI_TEXT);
    int visibleRows = (int)((height - 54) / 58);
    if (visibleRows < 1) visibleRows = 1;
    int maxScroll = nb_reservations > visibleRows ? nb_reservations - visibleRows : 0;
    if (app.invoiceScroll > maxScroll) app.invoiceScroll = maxScroll;
    if (inside(panel)) {
        float wheel = GetMouseWheelMove();
        if (wheel < 0 && app.invoiceScroll < maxScroll) app.invoiceScroll++;
        if (wheel > 0 && app.invoiceScroll > 0) app.invoiceScroll--;
    }
    int row = 0;
    for (int i = app.invoiceScroll; i < nb_reservations && row < visibleRows; i++, row++) {
        Reservation *reservation = &reservations[i];
        float rowY = y + 48 + row * 58.0f;
        Rectangle item = {x + 8, rowY, width - 16, 50};
        bool active = reservation->id == app.selectedInvoice;
        if (active) drawRounded(item, ColorAlpha(UI_INDIGO, 0.2f));
        uiLabel(TextFormat("Facture #%d", reservation->id),
                (Vector2){item.x + 10, item.y + 7}, UI_FONT_14, UI_TEXT);
        uiLabel(reservation->nom_client,
                (Vector2){item.x + 10, item.y + 28}, UI_FONT_12, UI_MUTED);
        if (inside(item) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            app.selectedInvoice = reservation->id;
            app.selectedReservation = reservation->id;
        }
    }
}

static void drawInvoices(void)
{
    float x = SIDEBAR_WIDTH + 28.0f;
    float width = (float)GetScreenWidth() - x - 28;
    float y = HEADER_HEIGHT + 8;
    float height = GetScreenHeight() - y - 24;
    float listWidth = clampf(width * 0.28f, 250, 330);
    drawInvoiceList(x, y, listWidth, height);

    Reservation *reservation = resTrouver(app.selectedInvoice);
    Rectangle preview = {x + listWidth + 16, y, width - listWidth - 16, height};
    uiPanel(preview, UI_SURFACE);
    if (reservation == NULL) {
        drawTextWrapped("Selectionnez une facture pour afficher son apercu.",
                        preview.x + 28, preview.y + 35, preview.width - 56,
                        UI_FONT_16, UI_MUTED);
        return;
    }
    DrawRectangleRounded((Rectangle){preview.x + 28, preview.y + 24,
        preview.width - 56, preview.height - 48}, 0.04f, 4,
        (Color){248, 250, 252, 255});
    float pageX = preview.x + 62, pageY = preview.y + 54;
    uiLabel("ORBITE", (Vector2){pageX, pageY}, UI_FONT_28, UI_INDIGO);
    uiLabel("FACTURE DE RESERVATION", (Vector2){pageX, pageY + 40},
            UI_FONT_14, (Color){71, 85, 105, 255});
    DrawLineEx((Vector2){pageX, pageY + 68},
               (Vector2){preview.x + preview.width - 62, pageY + 68},
               2, UI_INDIGO);
    uiLabel(TextFormat("FACTURE N  %d", reservation->id),
            (Vector2){pageX, pageY + 90}, UI_FONT_16, (Color){30, 41, 59, 255});
    const char *labels[] = {"Client", "Salle", "Date", "Horaire",
                            "Duree", "Participants", "Montant"};
    char values[7][100];
    snprintf(values[0], sizeof(values[0]), "%s", reservation->nom_client);
    snprintf(values[1], sizeof(values[1]), "%s", reservation->salle.nom);
    snprintf(values[2], sizeof(values[2]), "%s", reservation->date);
    snprintf(values[3], sizeof(values[3]), "%s - %s",
             reservation->heure_debut, reservation->heure_fin);
    int duration = heureEnMinutes(reservation->heure_fin) -
                   heureEnMinutes(reservation->heure_debut);
    snprintf(values[4], sizeof(values[4]), "%d h %02d min",
             duration / 60, duration % 60);
    snprintf(values[5], sizeof(values[5]), "%d", reservation->nombre_personnes);
    char total[32];
    moneyText(reservation->tarif, total, sizeof(total));
    snprintf(values[6], sizeof(values[6]), "%s TND", total);
    for (int i = 0; i < 7; i++) {
        float rowY = pageY + 140 + i * 42;
        uiLabel(labels[i], (Vector2){pageX, rowY}, UI_FONT_12,
                (Color){100, 116, 139, 255});
        uiLabel(values[i], (Vector2){pageX + 150, rowY}, UI_FONT_14,
                i == 6 ? UI_INDIGO : (Color){30, 41, 59, 255});
    }
    DrawLineEx((Vector2){pageX, pageY + 449},
               (Vector2){preview.x + preview.width - 62, pageY + 449},
               1, (Color){203, 213, 225, 255});
    uiLabel("Merci pour votre confiance.", (Vector2){pageX, pageY + 466},
            UI_FONT_12, (Color){71, 85, 105, 255});
    Rectangle openButton = {preview.x + preview.width - 248,
                            preview.y + preview.height - 66, 208, 42};
    if (uiButton(openButton, "Ouvrir le fichier .txt", UI_BUTTON_PRIMARY, true)) {
        char path[260];
        char url[320];
        snprintf(path, sizeof(path), "%s/data/Facture_%d.txt",
                 GetWorkingDirectory(), reservation->id);
        for (size_t i = 0; path[i] != '\0'; i++) {
            if (path[i] == '\\') path[i] = '/';
        }
        snprintf(url, sizeof(url), "file:///%s", path);
        OpenURL(url);
    }
}

static void drawModalBackdrop(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                  ColorAlpha((Color){2, 6, 23, 255}, 0.78f));
}

static void drawRoomModal(void)
{
    float width = fminf(540, GetScreenWidth() - 50);
    float height = 510;
    Rectangle modal = {(GetScreenWidth() - width) / 2,
                       (GetScreenHeight() - height) / 2, width, height};
    uiPanel(modal, UI_SURFACE_RAISED);
    uiLabel(app.editingRoom >= 0 ? "Modifier la salle" : "Ajouter une salle",
            (Vector2){modal.x + 28, modal.y + 24}, UI_FONT_20, UI_TEXT);
    uiLabel("Nom", (Vector2){modal.x + 28, modal.y + 80}, UI_FONT_14, UI_TEXT);
    uiTextInput((Rectangle){modal.x + 28, modal.y + 106, width - 56, 42},
                app.roomName, sizeof(app.roomName), "Nom de l'espace", false);
    uiLabel("Capacite (personnes)", (Vector2){modal.x + 28, modal.y + 164},
            UI_FONT_14, UI_TEXT);
    uiTextInput((Rectangle){modal.x + 28, modal.y + 190, 200, 42},
                app.roomCapacityText, sizeof(app.roomCapacityText), "Ex. 20", true);
    uiLabel("Tarif horaire (TND)", (Vector2){modal.x + 260, modal.y + 164},
            UI_FONT_14, UI_TEXT);
    uiTextInput((Rectangle){modal.x + 260, modal.y + 190, width - 288, 42},
                app.roomRateText, sizeof(app.roomRateText), "Ex. 25,00", true);
    uiLabel("Equipements disponibles", (Vector2){modal.x + 28, modal.y + 260},
            UI_FONT_14, UI_TEXT);
    uiCheckbox((Rectangle){modal.x + 28, modal.y + 290, 150, 34},
               "Wi-Fi", &app.roomWifi);
    uiCheckbox((Rectangle){modal.x + 192, modal.y + 290, 170, 34},
               "Projecteur", &app.roomProjector);
    uiCheckbox((Rectangle){modal.x + 28, modal.y + 334, 190, 34},
               "Tableau blanc", &app.roomWhiteboard);
    uiLabel("La modification met a jour les details des reservations liees.",
            (Vector2){modal.x + 28, modal.y + 386}, UI_FONT_12, UI_MUTED);

    if (uiButton((Rectangle){modal.x + 28, modal.y + height - 66, 130, 40},
                 "Annuler", UI_BUTTON_SECONDARY, true)) app.modal = MODAL_NONE;
    if (uiButton((Rectangle){modal.x + width - 190, modal.y + height - 66, 162, 40},
                 "Enregistrer", UI_BUTTON_PRIMARY, true)) {
        int capacity;
        float rate;
        if (app.roomName[0] == '\0' ||
            !parsePositiveInt(app.roomCapacityText, &capacity) ||
            !parseMoney(app.roomRateText, &rate)) {
            notify("Verifiez le nom, la capacite et le tarif.", UI_ERROR);
            return;
        }
        char equipment[200] = "";
        if (app.roomWifi) strcat(equipment, "Wi-Fi");
        if (app.roomProjector) {
            if (equipment[0]) strcat(equipment, ", ");
            strcat(equipment, "Projecteur");
        }
        if (app.roomWhiteboard) {
            if (equipment[0]) strcat(equipment, ", ");
            strcat(equipment, "Tableau blanc");
        }
        ResCode code = app.editingRoom >= 0
            ? salleModifier(salles[app.editingRoom].nom, app.roomName,
                            capacity, rate, equipment)
            : salleCreer(app.roomName, capacity, rate, equipment);
        notifyResult(code);
        if (code == RES_OK) app.modal = MODAL_NONE;
    }
}

static void drawConfirmModal(void)
{
    float width = fminf(440, GetScreenWidth() - 48);
    Rectangle modal = {(GetScreenWidth() - width) / 2,
                       (GetScreenHeight() - 210) / 2, width, 210};
    uiPanel(modal, UI_SURFACE_RAISED);
    const char *title = app.modal == MODAL_DELETE_ROOM ?
        "Supprimer cette salle ?" : "Confirmer l'action";
    uiLabel(title, (Vector2){modal.x + 24, modal.y + 24}, UI_FONT_20, UI_TEXT);
    const char *description = app.modal == MODAL_DELETE_ROOM ?
        "Les reservations et factures historiques seront conservees." :
        app.action == ACTION_CANCEL_RESERVATION ?
        "La reservation restera dans l'historique avec le statut annulee." :
        "La reservation et son fichier de facture seront supprimes.";
    drawTextWrapped(description, modal.x + 24, modal.y + 67, width - 48,
                    UI_FONT_14, UI_MUTED);
    if (uiButton((Rectangle){modal.x + 24, modal.y + 142, 132, 40},
                 "Retour", UI_BUTTON_SECONDARY, true)) app.modal = MODAL_NONE;
    if (uiButton((Rectangle){modal.x + width - 178, modal.y + 142, 154, 40},
                 "Confirmer", UI_BUTTON_DANGER, true)) {
        ResCode code;
        if (app.modal == MODAL_DELETE_ROOM) {
            code = salleSupprimer(salles[app.selectedRoom].nom);
            if (code == RES_OK) app.selectedRoom = -1;
        } else if (app.action == ACTION_CANCEL_RESERVATION) {
            code = resAnnuler(app.selectedReservation);
        } else {
            code = resSupprimer(app.selectedReservation);
            if (code == RES_OK) app.selectedReservation = -1;
        }
        notifyResult(code);
        app.modal = MODAL_NONE;
    }
}

static void drawModal(void)
{
    if (app.modal == MODAL_NONE) return;
    drawModalBackdrop();
    if (app.modal == MODAL_ROOM_FORM) drawRoomModal();
    else drawConfirmModal();
}

static void handleKeyboard(void)
{
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (app.modal != MODAL_NONE) app.modal = MODAL_NONE;
        else if (app.page == PAGE_BOOKING && app.wizardStep > 0) app.wizardStep--;
    }
}

static void drawApp(void)
{
    BeginDrawing();
    ClearBackground(UI_BG);
    uiWidgetsBeginFrame();
    drawSidebar();
    drawHeader();

    switch (app.page) {
        case PAGE_DASHBOARD: drawDashboard(); break;
        case PAGE_ROOMS: drawRooms(); break;
        case PAGE_BOOKING: drawBooking(); break;
        case PAGE_RESERVATIONS: drawReservations(); break;
        case PAGE_PLANNING: drawPlanning(); break;
        case PAGE_INVOICES: drawInvoices(); break;
        default: break;
    }
    drawModal();
    if (app.toastSeconds > 0.0f) {
        app.toastSeconds -= GetFrameTime();
        uiToast(app.toastMessage, app.toastColor, app.toastSeconds);
    }
    uiWidgetsEndFrame();
    EndDrawing();
}

static Font loadInterFont(void)
{
    int glyphs[230];
    int count = 0;
    for (int codepoint = 32; codepoint <= 126; codepoint++) glyphs[count++] = codepoint;
    for (int codepoint = 160; codepoint <= 255; codepoint++) {
        if (codepoint != 173) glyphs[count++] = codepoint;
    }
    glyphs[count++] = 0x0152;
    glyphs[count++] = 0x0153;
    glyphs[count++] = 0x20AC;
    Font font = LoadFontEx("gui/assets/fonts/InterVariable.ttf", 36, glyphs, count);
    if (font.texture.id == 0) {
        TraceLog(LOG_WARNING, "Inter not loaded; using raylib default font.");
        font = GetFontDefault();
    }
    return font;
}

static void captureFrame(void)
{
    static const char *names[PAGE_COUNT] = {
        "01-dashboard.png", "02-salles.png", "03-reservation.png",
        "04-reservations.png", "05-planning.png", "06-factures.png"
    };
    if (app.captureDelayFrames > 0) {
        app.captureDelayFrames--;
        return;
    }
    TakeScreenshot(TextFormat("images/captures/%s", names[app.captureIndex]));
    app.captureIndex++;
    if (app.captureIndex >= PAGE_COUNT) {
        app.captureScreenshots = false;
        app.captureExit = true;
    } else {
        app.page = (Page)app.captureIndex;
        if (app.page == PAGE_BOOKING) startBooking(-1);
        app.captureDelayFrames = 1;
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    setlocale(LC_ALL, "");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(1280, 760, "ORBITE | Gestion des salles et reservations");
    if (!IsWindowReady()) return 1;
    SetWindowMinSize(1100, 680);
    SetTargetFPS(60);
    app.font = loadInterFont();
    uiWidgetsSetFont(app.font);
    Image icon = LoadImage("gui/assets/brand/orbite.png");
    if (icon.data != NULL) {
        SetWindowIcon(icon);
        UnloadImage(icon);
    } else {
        TraceLog(LOG_WARNING, "ORBITE icon not found at gui/assets/brand/orbite.png");
    }
    app.brandMark = LoadTexture("gui/assets/brand/orbite.png");
    if (app.brandMark.id == 0) {
        TraceLog(LOG_ERROR, "ORBITE logo not found at gui/assets/brand/orbite.png");
        if (app.font.texture.id != GetFontDefault().texture.id) UnloadFont(app.font);
        CloseWindow();
        return 1;
    }

    ResCode code = coreInitialiserSalles();
    if (code == RES_OK) code = coreChargerReservations();
    if (code != RES_OK) {
        TraceLog(LOG_ERROR, "%s", resCodeMessage(code));
        CloseWindow();
        return 1;
    }
    initializePeriod();
    app.page = PAGE_DASHBOARD;
    app.editingRoom = -1;
    app.editingReservation = -1;
    app.selectedWizardRoom = -1;
    app.selectedRoom = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--capture-screenshots") == 0) {
            app.captureScreenshots = true;
            app.page = PAGE_DASHBOARD;
            app.captureDelayFrames = 1;
        }
    }

    while (!WindowShouldClose() && !app.captureExit) {
        handleKeyboard();
        drawApp();
        if (app.captureScreenshots) captureFrame();
    }
    UnloadTexture(app.brandMark);
    if (app.font.texture.id != GetFontDefault().texture.id) UnloadFont(app.font);
    CloseWindow();
    return 0;
}
