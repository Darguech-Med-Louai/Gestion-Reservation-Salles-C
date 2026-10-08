#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "Structures.h"
#include "FonctionsAux.h"
#include "GestionSalle.h"
#include "Reservation.h"
#include "Stats.h"
#include "Menu.h"
#include "Facture.h"
#include "Interface.h"
#include "core/Core.h"

// ---------------------------
// Tableaux pour stocker salles et réservations
// ---------------------------

// ---------------------------
// Programme principal
// ---------------------------
int main(){
    setlocale(LC_ALL,"");
    uiInitialiser();


    ResCode code = coreInitialiserSalles();
    if (code != RES_OK) {
        fprintf(stderr, "Erreur d'initialisation des salles : %s\n",
                resCodeMessage(code));
        return 1;
    }


    // Charger les réservations depuis le fichier
    chargerReservations();

    menu();
    
    return 0;
}