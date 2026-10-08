#include <stdio.h>
#include <string.h>

#include "../src/Structures.h"
#include "../src/core/Core.h"

static int echec;

static void verifier(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "ECHEC: %s\n", message);
        echec = 1;
    }
}

int main(void)
{
    Reservation resultat;
    SallesDisponibles espaces;
    StatsResultat stats;

    strcpy(salles[0].nom, "Mercure");
    salles[0].capacite = 10;
    salles[0].tarif_horaire = 10.0f;
    strcpy(salles[0].equipements, "Wi-Fi");
    nb_salles = 1;
    verifier(coreSauvegarderSalles() == RES_OK, "sauvegarde du tarif");
    memset(salles, 0, sizeof(salles));
    nb_salles = 0;
    verifier(coreChargerSalles() == RES_OK && nb_salles == 1 &&
             salles[0].tarif_horaire == 10.0f,
             "rechargement du tarif a virgule");

    verifier(resCreer("Client A", 4, "08/11/2025", "09:00", "11:00",
                      "Mercure", &resultat) == RES_OK,
             "creation valide");
    verifier(resultat.id == 1 && resultat.tarif == 20.0f,
             "id initial et estimation du tarif");

    verifier(resCreer("Client B", 4, "08/11/2025", "10:00", "11:00",
                      "Mercure", &resultat) == RES_CONFLIT,
             "conflit de creneau");
    verifier(resCreer("Client B", 11, "08/11/2025", "12:00", "13:00",
                      "Mercure", &resultat) == RES_CAPACITE,
             "capacite insuffisante");
    verifier(resCreer("Client B", 4, "08/11/2025", "07:59", "09:00",
                      "Mercure", &resultat) == RES_HORAIRE_INVALIDE,
             "horaire avant l'ouverture");
    verifier(resCreer("Client B", 4, "31/02/2025", "12:00", "13:00",
                      "Mercure", &resultat) == RES_DATE_INVALIDE,
             "date impossible");

    verifier(resModifier(1, "Client A", 4, "08/11/2025", "09:30", "10:30",
                         "Mercure", &resultat) == RES_OK,
             "modification chevauchant l'ancien horaire sans auto-conflit");
    verifier(strcmp(resultat.statut, "modifiee") == 0 && resultat.tarif == 10.0f,
             "statut et tarif apres modification");

    reservations[nb_reservations] = reservations[0];
    reservations[nb_reservations].id = 7;
    strcpy(reservations[nb_reservations].nom_client, "Client historique");
    strcpy(reservations[nb_reservations].date, "15/11/2025");
    strcpy(reservations[nb_reservations].heure_debut, "09:00");
    strcpy(reservations[nb_reservations].heure_fin, "10:00");
    reservations[nb_reservations].tarif = 10.0f;
    strcpy(reservations[nb_reservations].statut, "confirmee");
    nb_reservations++;
    verifier(coreSauvegarderReservations() == RES_OK, "sauvegarde de donnees");

    memset(reservations, 0, sizeof(reservations));
    nb_reservations = 0;
    verifier(coreChargerReservations() == RES_OK && nb_reservations == 2,
             "rechargement apres redemarrage");
    verifier(reservations[0].tarif == 10.0f &&
             strcmp(reservations[0].salle.nom, "Mercure") == 0,
             "lecture du montant a virgule et restauration de la salle");

    verifier(resCreer("Client C", 3, "08/11/2025", "10:30", "11:30",
                      "Mercure", &resultat) == RES_OK && resultat.id == 8,
             "nouvel id egal au maximum existant plus un");
    verifier(resAnnuler(8) == RES_OK &&
             strcmp(reservations[2].statut, "annulee") == 0,
             "annulation conservee comme statut");
    verifier(sallesDisponibles(3, "08/11/2025", "10:30", "11:30",
                               &espaces) == RES_OK && espaces.nombre == 1,
             "un creneau annule redevient disponible");
    statsCalculer(11, 2025, &stats);
    verifier(stats.reservations == 2 && stats.annulees == 1 &&
             stats.heuresDisponibles == 480.0 && stats.heuresReservees == 2.0,
             "statistiques mensuelles et capacite horaire");

    verifier(resSupprimer(7) == RES_OK, "suppression physique");
    int idSupprime = 0;
    for (int i = 0; i < nb_reservations; i++) {
        if (reservations[i].id == 7) idSupprime = 1;
    }
    verifier(!idSupprime && nb_reservations == 2, "suppression du tableau");
    verifier(resCreer("Client D", 2, "08/11/2025", "11:30", "12:00",
                      "Mercure", &resultat) == RES_OK && resultat.id == 9,
             "aucun doublon apres suppression d'un identifiant");

    memset(reservations, 0, sizeof(reservations));
    nb_reservations = 0;
    verifier(coreChargerReservations() == RES_OK && nb_reservations == 3,
             "persistance des reservations apres redemarrage");
    verifier(strcmp(reservations[1].statut, "annulee") == 0,
             "persistance du statut annulee");
    verifier(salleCreer("Venus", 12, 18.5f, "Projecteur") == RES_OK,
             "creation d'une salle");
    verifier(salleModifier("Venus", "Venus Pro", 14, 20.0f,
                           "Projecteur, Wi-Fi") == RES_OK,
             "modification d'une salle");
    verifier(salleSupprimer("Venus Pro") == RES_OK && nb_salles == 1,
             "suppression d'une salle sans reservations");
    verifier(salleModifier("Mercure", "Mercure Pro", 12, 11.0f,
                           "Wi-Fi, Projecteur") == RES_OK,
             "renommage de salle avec reservations historiques");
    verifier(salleSupprimer("Mercure Pro") == RES_OK &&
             nb_salles == 0 && nb_reservations == 3 &&
             strcmp(reservations[0].salle.nom, "Mercure Pro") == 0,
             "suppression de salle conserve les reservations");

    if (echec) return 1;
    puts("Tous les tests du coeur ont reussi.");
    return 0;
}
