#ifndef INTERFACE_H
#define INTERFACE_H

void uiInitialiser(void);
int uiLireEntier(const char *invite, int minimum, int maximum);
void uiAfficherEntete(int nbSalles, int nbReservations);
void uiSection(const char *titre);
void uiOption(char numero, const char *texte);
void uiSucces(const char *message);
void uiErreur(const char *message);
void uiInfo(const char *message);
void uiSeparateur(void);

#endif
