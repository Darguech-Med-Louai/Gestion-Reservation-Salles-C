#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Menu.h"
#include "Interface.h"

static void creerReservation(void)
{
    char nomClient[50], nomSalle[50], date[11], debut[6], fin[6];
    int nbPersonnes;

    uiSection("NOUVELLE RESERVATION");
    printf("  Nom du client : ");
    if (fgets(nomClient, sizeof(nomClient), stdin) == NULL) return;
    nettoyerChaine(nomClient);
    while (nomClient[0] == '\0') {
        uiErreur("Le nom du client ne peut pas etre vide.");
        printf("  Nom du client : ");
        if (fgets(nomClient, sizeof(nomClient), stdin) == NULL) return;
        nettoyerChaine(nomClient);
    }

    nbPersonnes = uiLireEntier("Nombre de personnes : ", 1, 100000);
    if (nbPersonnes < 0) return;

    lireDate(date);
    lireHeure(debut, "Heure de debut (08:00-23:59)");
    while (heureEnMinutes(debut) < 480) {
        uiErreur("L'heure de debut doit etre comprise entre 08:00 et 23:59.");
        lireHeure(debut, "Heure de debut");
    }
    lireHeure(fin, "Heure de fin (apres le debut)");
    while (heureEnMinutes(fin) <= heureEnMinutes(debut)) {
        uiErreur("L'heure de fin doit etre apres l'heure de debut.");
        lireHeure(fin, "Heure de fin");
    }

    if (!recommanderSalles(nbPersonnes, date, debut, fin)) return;

    printf("\n  Salle souhaitee : ");
    if (fgets(nomSalle, sizeof(nomSalle), stdin) == NULL) return;
    nettoyerChaine(nomSalle);

    Salle *salle = trouverSalle(nomSalle);
    while (salle == NULL || salle->capacite < nbPersonnes ||
           !salleDisponible(*salle, date, debut, fin)) {
        uiErreur("Cette salle n'est pas disponible ou sa capacite est insuffisante.");
        printf("  Choisissez une salle proposee (ou saisissez 0 pour annuler) : ");
        if (fgets(nomSalle, sizeof(nomSalle), stdin) == NULL) return;
        nettoyerChaine(nomSalle);
        if (strcmp(nomSalle, "0") == 0) {
            uiInfo("Reservation abandonnee.");
            return;
        }
        salle = trouverSalle(nomSalle);
    }

    int minutes = heureEnMinutes(fin) - heureEnMinutes(debut);
    float tarif = (minutes / 60.0f) * salle->tarif_horaire;
    uiSection("RECAPITULATIF");
    printf("  Client       %s\n", nomClient);
    printf("  Salle        %s\n", salle->nom);
    printf("  Creneau      %s, %s - %s\n", date, debut, fin);
    printf("  Participants %d personne(s)\n", nbPersonnes);
    printf("  Duree        %d h %02d min\n", minutes / 60, minutes % 60);
    printf("  Total estime %.2f TND\n", tarif);

    uiSeparateur();
    uiOption('1', "Confirmer la reservation");
    uiOption('2', "Retourner au menu");
    int choix = uiLireEntier("Votre choix : ", 1, 2);
    if (choix < 0) return;
    if (choix == 1) {
        if (ajouterReservation(0, nomClient, nomSalle, date, debut, fin, nbPersonnes)) {
            uiSucces("Reservation confirmee et facture generee.");
        } else {
            uiErreur("La reservation n'a pas pu etre enregistree.");
        }
    } else {
        uiInfo("Reservation abandonnee.");
    }
}

void menu(void)
{
    int choix;

    do {
        uiAfficherEntete(nb_salles, nb_reservations);
        uiSection("MENU PRINCIPAL");
        uiOption('1', "Creer une reservation");
        uiOption('2', "Decouvrir les salles");
        uiOption('3', "Consulter les statistiques");
        uiOption('4', "Gerer une reservation");
        uiOption('5', "Quitter l'application");
        choix = uiLireEntier("Votre choix : ", 1, 5);
        if (choix < 0) choix = 5;

        switch (choix) {
            case 1:
                creerReservation();
                break;
            case 2:
                afficherSalles();
                break;
            case 3:
                statistiquesCompletes();
                break;
            case 4:
                annulerModifierReservation();
                break;
            case 5:
                uiInfo("Merci d'avoir choisi ORBITE. A bientot !");
                break;
        }
    } while (choix != 5);
}
