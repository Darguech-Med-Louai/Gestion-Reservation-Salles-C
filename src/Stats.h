#ifndef STATS_H
#define STATS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Structures.h"  // <-- indispensable !
#include "GestionSalle.h"
#include "Reservation.h"
#include "FonctionsAux.h"

// Déclarer les variables externes définies dans main.c
extern Salle salles[MAX_SALLES];
extern int nb_salles;
extern Reservation reservations[MAX_RES];
extern int nb_reservations;

// Déclarations des fonctions statistiques
void statistiquesCompletes();
int compterReservationsParMois(int mois, int annee);
void afficherSallesPopulaires(int mois, int annee);

#endif // STATS_H
