#ifndef ORBITE_PERSISTENCE_H
#define ORBITE_PERSISTENCE_H

#include <stdio.h>
#include "../Structures.h"

int persistenceChargerReservations(void);
int persistenceSauvegarderReservations(void);
int persistenceGenererFacture(const Reservation *reservation);
int persistenceChargerSalles(void);
int persistenceSauvegarderSalles(void);
int persistenceEcrireMontant(FILE *fichier, float montant);

#endif
