#ifndef GESTIONSALLE_H
#define GESTIONSALLE_H

#include "Structures.h"
#include "FonctionsAux.h"  // <- nécessaire pour salleDisponible, calculTarif, etc.

// Déclaration des variables globales (définies dans main.c)
extern Salle salles[MAX_SALLES];
extern int nb_salles;

// ---------------------------
// Gestion des salles
// ---------------------------

// Création ou mise à jour du fichier Tarif.txt
void creerFichierTarif();

// Ajouter une salle
void ajouterSalle(char nom[], int capacite, float tarif, char equipements[]);

// Afficher la liste des salles
void afficherSalles();

// Rechercher une salle par nom
Salle* trouverSalle(char nom_salle[]);

// ---------------------------
// Recommandation de salle
// ---------------------------

// Affiche les salles disponibles et retourne 1 si au moins une salle est trouvée, sinon 0
int recommanderSalles(int nb_personnes, char date[11], char debut[6], char fin[6]);

#endif // GESTIONSALLE_H
