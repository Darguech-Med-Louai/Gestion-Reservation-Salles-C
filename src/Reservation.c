#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "Reservation.h"
#include "Interface.h"
#include "core/Core.h"

static int lireEntierPositif(const char *invite)
{
    return uiLireEntier(invite, 1, 100000);
}

int ajouterReservation(int id, char nom_client[], char nom_salle[],
                      char date[11], char debut[6], char fin[6],
                      int nb_personnes)
{
    Reservation resultat;
    ResCode code;
    (void)id;

    code = resCreer(nom_client, nb_personnes, date, debut, fin, nom_salle, &resultat);
    if (code != RES_OK) {
        uiErreur(resCodeMessage(code));
        return 0;
    }
    return 1;
}

void annulerModifierReservation(void)
{
    if (nb_reservations == 0) {
        uiInfo("Aucune reservation a gerer.");
        return;
    }

    int id = uiLireEntier("ID de la reservation : ", 1, INT_MAX);
    if (id < 0) return;

    Reservation *reservation = trouverReservation(id);
    if (reservation == NULL) {
        uiErreur(resCodeMessage(RES_INTROUVABLE));
        return;
    }

    printf("\n  Reservation #%d | %s | %s | %s | %s-%s\n",
           reservation->id, reservation->nom_client, reservation->salle.nom,
           reservation->date, reservation->heure_debut, reservation->heure_fin);
    uiOption('1', "Annuler la reservation");
    uiOption('2', "Modifier la reservation");
    int choix = uiLireEntier("Votre choix : ", 1, 2);
    if (choix < 0) return;

    if (choix == 1) {
        ResCode code = resAnnuler(id);
        if (code == RES_OK) uiSucces("Reservation annulee.");
        else uiErreur(resCodeMessage(code));
        return;
    }

    int personnes = lireEntierPositif("Nombre de personnes : ");
    if (personnes < 0) return;

    char date[11], debut[6], fin[6], nomSalle[50];
    lireDate(date);
    lireHeure(debut, "Heure debut");
    lireHeure(fin, "Heure fin");
    SallesDisponibles disponibles;
    ResCode code = sallesDisponibles(personnes, date, debut, fin, &disponibles);
    if (code != RES_OK) {
        uiErreur(resCodeMessage(code));
        return;
    }
    printf("\n  Espaces adaptes au creneau :\n");
    for (size_t i = 0; i < disponibles.nombre; i++) {
        printf("  %-12s | %2d places | %.2f TND/h | %s\n",
               disponibles.elements[i].nom, disponibles.elements[i].capacite,
               disponibles.elements[i].tarif_horaire, disponibles.elements[i].equipements);
    }

    printf("  Salle souhaitee (0 pour annuler) : ");
    if (fgets(nomSalle, sizeof(nomSalle), stdin) == NULL) return;
    nettoyerChaine(nomSalle);
    if (strcmp(nomSalle, "0") == 0) return;

    Reservation ancienne = *reservation;
    Salle *salle = trouverSalle(nomSalle);
    if (salle == NULL) {
        uiErreur(resCodeMessage(RES_SALLE_INCONNUE));
        return;
    }
    int minutes = heureEnMinutes(fin) - heureEnMinutes(debut);
    printf("  Estimation : %.2f TND (%d h %02d min)\n",
           (minutes / 60.0f) * salle->tarif_horaire, minutes / 60, minutes % 60);
    uiOption('1', "Confirmer les modifications");
    uiOption('2', "Retourner au menu");
    choix = uiLireEntier("Votre choix : ", 1, 2);
    if (choix != 1) return;

    Reservation resultat;
    code = resModifier(id, ancienne.nom_client, personnes, date, debut, fin,
                       nomSalle, &resultat);
    if (code == RES_OK) uiSucces("Reservation modifiee.");
    else uiErreur(resCodeMessage(code));
}

Reservation *trouverReservation(int id)
{
    for (int i = 0; i < nb_reservations; i++) {
        if (reservations[i].id == id) return &reservations[i];
    }
    return NULL;
}

int supprimerReservation(int id)
{
    ResCode code = resSupprimer(id);
    if (code != RES_OK) {
        uiErreur(resCodeMessage(code));
        return 0;
    }
    return 1;
}

void afficherReservations(void)
{
    if (nb_reservations == 0) {
        uiInfo("Aucune reservation enregistree.");
        return;
    }
    uiSection("RESERVATIONS");
    for (int i = 0; i < nb_reservations; i++) {
        printf("  #%d | %-24s | %-10s | %s | %s-%s | %s | %.2f TND\n",
               reservations[i].id, reservations[i].nom_client,
               reservations[i].salle.nom, reservations[i].date,
               reservations[i].heure_debut, reservations[i].heure_fin,
               reservations[i].statut, reservations[i].tarif);
    }
}

void sauvegarderReservations(void)
{
    if (coreSauvegarderReservations() != RES_OK) {
        uiErreur("Erreur lors de la sauvegarde des reservations.");
    }
}

void chargerReservations(void)
{
    if (coreChargerReservations() != RES_OK) {
        uiErreur("Le fichier Reservations.txt est invalide ou illisible.");
    }
}
