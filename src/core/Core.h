#ifndef ORBITE_CORE_H
#define ORBITE_CORE_H

#include <stddef.h>
#include "../Structures.h"

typedef enum {
    RES_OK = 0,
    RES_SALLE_INCONNUE,
    RES_CAPACITE,
    RES_CONFLIT,
    RES_HORAIRE_INVALIDE,
    RES_DATE_INVALIDE,
    RES_NOM_INVALIDE,
    RES_NOMBRE_INVALIDE,
    RES_INTROUVABLE,
    RES_CAPACITE_MAX,
    RES_SALLE_OCCUPEE,
    RES_ERREUR_FICHIER,
    RES_ARGUMENT_INVALIDE,
    RES_SALLE_EXISTANTE
} ResCode;

typedef struct {
    Salle elements[MAX_SALLES];
    size_t nombre;
} SallesDisponibles;

typedef struct {
    int mois;
    int annee;
    int reservations;
    int annulees;
    float chiffreAffaires;
    int reservationsParMois[12];
    float chiffreAffairesParSalle[MAX_SALLES];
    int reservationsParSalle[MAX_SALLES];
    double heuresReservees;
    double heuresDisponibles;
} StatsResultat;

ResCode resCreer(const char *client, int personnes, const char *date,
                 const char *debut, const char *fin, const char *nomSalle,
                 Reservation *resultat);
ResCode resModifier(int id, const char *client, int personnes, const char *date,
                    const char *debut, const char *fin, const char *nomSalle,
                    Reservation *resultat);
ResCode resAnnuler(int id);
ResCode resSupprimer(int id);
Reservation *resTrouver(int id);
ResCode salleCreer(const char *nom, int capacite, float tarif,
                   const char *equipements);
ResCode salleModifier(const char *ancienNom, const char *nom, int capacite,
                      float tarif, const char *equipements);
ResCode salleSupprimer(const char *nom);

ResCode sallesDisponibles(int personnes, const char *date, const char *debut,
                          const char *fin, SallesDisponibles *resultat);
void statsCalculer(int mois, int annee, StatsResultat *resultat);
ResCode coreInitialiserSalles(void);
ResCode coreChargerSalles(void);
ResCode coreSauvegarderSalles(void);
ResCode coreChargerReservations(void);
ResCode coreSauvegarderReservations(void);
const char *resCodeMessage(ResCode code);

#endif
