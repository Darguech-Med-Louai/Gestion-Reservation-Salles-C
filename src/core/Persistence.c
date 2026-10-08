#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Persistence.h"
#include "../FonctionsAux.h"

extern Salle salles[MAX_SALLES];
extern int nb_salles;
extern Reservation reservations[MAX_RES];
extern int nb_reservations;

static void retirerEspaces(char *texte)
{
    size_t longueur = strlen(texte);
    size_t debut = 0;

    while (debut < longueur && isspace((unsigned char)texte[debut])) {
        debut++;
    }
    if (debut != 0) {
        memmove(texte, texte + debut, longueur - debut + 1);
    }
    longueur = strlen(texte);
    while (longueur > 0 && isspace((unsigned char)texte[longueur - 1])) {
        texte[--longueur] = '\0';
    }
}

static int lireMontant(const char *texte, float *montant)
{
    char *fin;
    long entiere;
    long centimes = 0;
    int chiffres = 0;

    entiere = strtol(texte, &fin, 10);
    if (fin == texte || (*fin != ',' && *fin != '.')) {
        return 0;
    }
    fin++;
    while (isdigit((unsigned char)*fin) && chiffres < 2) {
        centimes = centimes * 10 + (*fin - '0');
        chiffres++;
        fin++;
    }
    if (*fin != '\0' || chiffres == 0) {
        return 0;
    }
    if (chiffres == 1) {
        centimes *= 10;
    }
    *montant = (float)entiere + (float)centimes / 100.0f;
    return 1;
}

int persistenceSauvegarderSalles(void)
{
    FILE *fichier = fopen("data/Tarif.txt", "w");
    if (fichier == NULL) {
        return 0;
    }
    for (int i = 0; i < nb_salles; i++) {
        if (fprintf(fichier, "%s | Capacite: %d | Tarif: ", salles[i].nom,
                    salles[i].capacite) < 0 ||
            !persistenceEcrireMontant(fichier, salles[i].tarif_horaire) ||
            fprintf(fichier, " TND/h | Equipements: %s\n",
                    salles[i].equipements) < 0) {
            fclose(fichier);
            return 0;
        }
    }
    return fclose(fichier) == 0;
}

int persistenceChargerSalles(void)
{
    FILE *fichier = fopen("data/Tarif.txt", "r");
    Salle lues[MAX_SALLES] = {0};
    int nombre = 0;

    if (fichier == NULL) {
        nb_salles = 0;
        return 1;
    }

    while (nombre < MAX_SALLES) {
        Salle salle = {0};
        char montant[32];
        int lus = fscanf(fichier,
            " %49[^|] | Capacite: %d | Tarif: %31[^ ] TND/h | Equipements: %199[^\n]",
            salle.nom, &salle.capacite, montant, salle.equipements);
        if (lus == EOF) break;
        if (lus != 4 || !lireMontant(montant, &salle.tarif_horaire) ||
            salle.capacite <= 0 || salle.tarif_horaire < 0.0f) {
            fclose(fichier);
            return 0;
        }
        retirerEspaces(salle.nom);
        retirerEspaces(salle.equipements);
        lues[nombre++] = salle;
        int finLigne;
        do {
            finLigne = fgetc(fichier);
        } while (finLigne != '\n' && finLigne != EOF);
    }
    if (ferror(fichier) || fclose(fichier) != 0) {
        return 0;
    }
    memcpy(salles, lues, sizeof(Salle) * (size_t)nombre);
    nb_salles = nombre;
    return 1;
}

int persistenceEcrireMontant(FILE *fichier, float montant)
{
    long long centimes = llround((double)montant * 100.0);
    long long absolu = centimes < 0 ? -centimes : centimes;

    if (centimes < 0 && fputc('-', fichier) == EOF) {
        return 0;
    }
    return fprintf(fichier, "%lld,%02lld", absolu / 100, absolu % 100) >= 0;
}

int persistenceSauvegarderReservations(void)
{
    FILE *fichier = fopen("data/Reservations.txt", "w");
    if (fichier == NULL) {
        return 0;
    }

    for (int i = 0; i < nb_reservations; i++) {
        int ecrit = fprintf(fichier,
            "ID: %d | Client: %s | Salle: %s | Date: %s | Début: %s | Fin: %s | Personnes: %d | Tarif: ",
            reservations[i].id,
            reservations[i].nom_client,
            reservations[i].salle.nom,
            reservations[i].date,
            reservations[i].heure_debut,
            reservations[i].heure_fin,
            reservations[i].nombre_personnes);
        if (ecrit < 0 ||
            !persistenceEcrireMontant(fichier, reservations[i].tarif) ||
            fprintf(fichier, " TND | Statut: %s\n", reservations[i].statut) < 0) {
            fclose(fichier);
            return 0;
        }
    }

    return fclose(fichier) == 0;
}

int persistenceChargerReservations(void)
{
    FILE *fichier = fopen("data/Reservations.txt", "r");
    int nombre = 0;

    nb_reservations = 0;
    if (fichier == NULL) {
        return 1;
    }

    while (nombre < MAX_RES) {
        Reservation entree = {0};
        char montant[32];
        int lus = fscanf(fichier,
            "ID: %d | Client: %49[^|] | Salle: %49[^|] | Date: %10[^|] | Début: %5[^|] | Fin: %5[^|] | Personnes: %d | Tarif: %31[^ ] TND | Statut: %19[^\n]\n",
            &entree.id,
            entree.nom_client,
            entree.salle.nom,
            entree.date,
            entree.heure_debut,
            entree.heure_fin,
            &entree.nombre_personnes,
            montant,
            entree.statut);

        if (lus == EOF) {
            break;
        }
        if (lus != 9 || !lireMontant(montant, &entree.tarif)) {
            fclose(fichier);
            return 0;
        }

        retirerEspaces(entree.nom_client);
        retirerEspaces(entree.salle.nom);
        retirerEspaces(entree.date);
        retirerEspaces(entree.heure_debut);
        retirerEspaces(entree.heure_fin);
        retirerEspaces(entree.statut);
        for (int i = 0; i < nb_salles; i++) {
            if (strcmp(entree.salle.nom, salles[i].nom) == 0) {
                entree.salle = salles[i];
                break;
            }
        }
        reservations[nombre++] = entree;
    }

    if (ferror(fichier)) {
        fclose(fichier);
        return 0;
    }
    if (fclose(fichier) != 0) {
        return 0;
    }
    nb_reservations = nombre;
    return 1;
}

int persistenceGenererFacture(const Reservation *reservation)
{
    char nomFichier[64];
    FILE *fichier;
    int duree;
    int ecrit;

    if (reservation == NULL) {
        return 0;
    }
    snprintf(nomFichier, sizeof(nomFichier), "data/Facture_%d.txt",
             reservation->id);
    fichier = fopen(nomFichier, "w");
    if (fichier == NULL) {
        return 0;
    }
    duree = heureEnMinutes(reservation->heure_fin) -
             heureEnMinutes(reservation->heure_debut);
    ecrit = fprintf(fichier,
        "===== FACTURE =====\n"
        "Client : %s\n"
        "Salle : %s\n"
        "Date : %s\n"
        "Heure : %s-%s\n"
        "Duree : %d h %d min\n"
        "Montant : ",
        reservation->nom_client,
        reservation->salle.nom,
        reservation->date,
        reservation->heure_debut,
        reservation->heure_fin,
        duree / 60,
        duree % 60);
    if (ecrit < 0 || !persistenceEcrireMontant(fichier, reservation->tarif) ||
        fprintf(fichier, " TND\n") < 0) {
        fclose(fichier);
        return 0;
    }
    if (fclose(fichier) != 0) {
        return 0;
    }
    return ecrit >= 0;
}
