#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include "Interface.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

static int couleursActives = 0;

static void couleur(const char *code)
{
    if (couleursActives) {
        printf("\033[%sm", code);
    }
}

void uiInitialiser(void)
{
#ifdef _WIN32
    HANDLE sortie = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    if (sortie != INVALID_HANDLE_VALUE && GetConsoleMode(sortie, &mode)) {
        couleursActives = SetConsoleMode(sortie, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
    }
#else
    couleursActives = isatty(STDOUT_FILENO) && getenv("TERM") != NULL;
#endif
}

int uiLireEntier(const char *invite, int minimum, int maximum)
{
    char ligne[64];

    for (;;) {
        char *fin;
        long valeur;

        printf("  %s", invite);
        if (fgets(ligne, sizeof(ligne), stdin) == NULL) {
            return -1;
        }

        errno = 0;
        valeur = strtol(ligne, &fin, 10);
        while (isspace((unsigned char)*fin)) fin++;
        if (fin != ligne && errno != ERANGE && *fin == '\0' &&
            valeur >= minimum && valeur <= maximum &&
            valeur >= INT_MIN && valeur <= INT_MAX) {
            return (int)valeur;
        }
        uiErreur("Saisie invalide. Choisissez une option affichee.");
    }
}

void uiAfficherEntete(int nbSalles, int nbReservations)
{
    couleur("36;1");
    printf("\n+----------------------------------------------------------------------+\n");
    printf("|  ORBITE  |  GESTION DES ESPACES                                      |\n");
    couleur("0");
    printf("|  Reservation, organisation et pilotage de vos salles                |\n");
    couleur("2");
    printf("|  %d salle(s)  |  %d reservation(s) enregistree(s)                     |\n",
           nbSalles, nbReservations);
    couleur("0");
    couleur("36;1");
    printf("+----------------------------------------------------------------------+\n");
    couleur("0");
}

void uiSection(const char *titre)
{
    couleur("36;1");
    printf("\n-- %s --\n", titre);
    couleur("0");
}

void uiOption(char numero, const char *texte)
{
    couleur("36;1");
    printf("  [%c] ", numero);
    couleur("0");
    printf("%s\n", texte);
}

void uiSucces(const char *message)
{
    couleur("32;1");
    printf("  + ");
    couleur("0");
    printf("%s\n", message);
}

void uiErreur(const char *message)
{
    couleur("31;1");
    printf("  ! ");
    couleur("0");
    printf("%s\n", message);
}

void uiInfo(const char *message)
{
    couleur("36");
    printf("  > ");
    couleur("0");
    printf("%s\n", message);
}

void uiSeparateur(void)
{
    couleur("2");
    printf("  --------------------------------------------------------------------\n");
    couleur("0");
}
