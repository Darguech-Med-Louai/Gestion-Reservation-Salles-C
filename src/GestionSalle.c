#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GestionSalle.h"
#include "Interface.h"
#include "core/Core.h"
#include "core/Persistence.h"


// ---------------------------
// Gestion des salles
// ---------------------------
//ajourer les salles dans un fichier tarif.txt
void creerFichierTarif() {
    if (coreSauvegarderSalles() != RES_OK) {
        uiErreur("Erreur lors de l'ecriture du fichier Tarif.txt.");
    }
}
void ajouterSalle(char nom[], int capacite, float tarif, char equipements[]) {
    ResCode code = salleCreer(nom, capacite, tarif, equipements);
    if (code != RES_OK) uiErreur(resCodeMessage(code));
}

void afficherSalles() {
    uiSection("NOS ESPACES");
    if (nb_salles == 0) {
        uiInfo("Aucune salle n'est enregistree.");
        return;
    }
    for(int i=0;i<nb_salles;i++){
        printf("\n  %02d  ", i + 1);
        printf("%-12s  ", salles[i].nom);
        printf("%2d places  |  %6.2f TND/h\n", salles[i].capacite, salles[i].tarif_horaire);
        printf("      Equipements : %s\n", salles[i].equipements);
    }
    uiSeparateur();
}



// ---------------------------
// Recherche d'une salle
// ---------------------------
Salle* trouverSalle(char nom_salle[]) {
    for(int i=0; i<nb_salles; i++){
        if(strcmp(nom_salle, salles[i].nom)==0)
            return &salles[i];
    }
    return NULL;
}

// ---------------------------
// Recommandation de salle
// ---------------------------
int recommanderSalles(int nb_personnes, char date[11], char debut[6], char fin[6]) {

    uiSection("ESPACES DISPONIBLES");
    printf("  %d personne(s)  |  %s  |  %s-%s\n",
           nb_personnes, date, debut, fin);

    SallesDisponibles resultat;
    ResCode code = sallesDisponibles(nb_personnes, date, debut, fin, &resultat);
    if (code != RES_OK) {
        uiErreur(resCodeMessage(code));
        return 0;
    }

    for (size_t i = 0; i < resultat.nombre; i++) {
        printf("  %02d  %-12s  %2d places  |  %6.2f TND/h  |  %s\n",
               (int)i + 1, resultat.elements[i].nom,
               resultat.elements[i].capacite, resultat.elements[i].tarif_horaire,
               resultat.elements[i].equipements);
    }
    return 1;
}