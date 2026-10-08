#ifndef FACTURE_H
#define FACTURE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Structures.h"     
#include "FonctionsAux.h"   
#include "GestionSalle.h"  
#include "Reservation.h"

// Variables globales définies dans main.c
extern Reservation reservations[MAX_RES];
extern int nb_reservations;
// generer une nouvelle facture
void genererFacture(Reservation res);

#endif // RESERVATION_H