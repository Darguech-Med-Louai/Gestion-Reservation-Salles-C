#ifndef FONCTIONSAUX_H
#define FONCTIONSAUX_H

#include "Structures.h"

// ---------------------------
// Fonctions auxiliaires
// ---------------------------

// Convertit une heure "HH:MM" en minutes totales depuis minuit
int heureEnMinutes(const char heure[6]);

// Vérifie si deux créneaux horaires se chevauchent
int chevauche(const char debut1[6], const char fin1[6],
              const char debut2[6], const char fin2[6]);

// Calcule le tarif d'une réservation
float calculTarif(Salle salle, const char debut[6], const char fin[6]);

// Vérifie si une salle est disponible pour une date et un créneau
int salleDisponible(Salle salle, const char date[11],
                    const char debut[6], const char fin[6]);

// Nettoyage des chaînes : supprime le '\n' de fgets
void nettoyerChaine(char *str);

// ---------------------------
// Lecture sécurisée de la date et de l'heure
// ---------------------------

// Lecture d'une date au format JJ/MM/AAAA
void lireDate(char date[11]);

// Lecture d'une heure au format HH:MM
void lireHeure(char heure[6], const char *message);

#endif // FONCTIONSAUX_H
