#ifndef RESERVATION_H
#define RESERVATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Structures.h"     // Contient Salle et Reservation
#include "FonctionsAux.h"   // Pour salleDisponible, calculTarif, heureEnMinutes
#include "GestionSalle.h"   // Pour trouverSalle
#include "Menu.h"         // Pour lireDate et lireHeure
#include "Stats.h"       // Pour recommanderSalles
#include "Facture.h"      // Pour genererFacture

// Variables globales définies dans main.c
extern Reservation reservations[MAX_RES];
extern int nb_reservations;

// ---------------------------
// Déclarations des fonctions
// ---------------------------

// Ajouter une réservation
int ajouterReservation(int id, char nom_client[], char nom_salle[], char date[11],char debut[6], char fin[6], int nb_personnes);

// Afficher toutes les réservations
void afficherReservations();

// Rechercher une réservation par ID
Reservation* trouverReservation(int id);

// Supprimer une réservation par ID
int supprimerReservation(int id);

// Sauvegarder toutes les réservations dans un fichier Reservations.txt
void sauvegarderReservations();
// Charger les réservations depuis le fichier Reservations.txt
void chargerReservations();
// Annulation ou modification
void annulerModifierReservation();

#endif // RESERVATION_H
