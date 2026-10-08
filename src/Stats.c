#include <stdio.h>
#include <string.h>

#include "Stats.h"
#include "Interface.h"
#include "core/Core.h"

int compterReservationsParMois(int mois, int annee)
{
    StatsResultat resultat;
    statsCalculer(mois, annee, &resultat);
    return resultat.reservations;
}

void afficherSallesPopulaires(int mois, int annee)
{
    StatsResultat resultat;
    int maximum = 0;

    statsCalculer(mois, annee, &resultat);
    uiSection("CLASSEMENT DES ESPACES");
    printf("  Periode : %02d/%04d\n", mois, annee);
    for (int i = 0; i < nb_salles; i++) {
        if (resultat.reservationsParSalle[i] > maximum) {
            maximum = resultat.reservationsParSalle[i];
        }
    }
    if (maximum == 0) {
        uiInfo("Aucune reservation pour cette periode.");
        return;
    }
    for (int i = 0; i < nb_salles; i++) {
        if (resultat.reservationsParSalle[i] == maximum) {
            printf("  %s | %d reservation(s)\n", salles[i].nom, maximum);
        }
    }
}

void statistiquesCompletes(void)
{
    int choix;

    do {
        StatsResultat resultat;
        uiSection("TABLEAU DE BORD");
        uiOption('1', "Chiffre d'affaires par salle");
        uiOption('2', "Reservations par mois");
        uiOption('3', "Salle(s) la plus populaire(s)");
        uiOption('0', "Retour au menu principal");
        choix = uiLireEntier("Votre choix : ", 0, 3);
        if (choix < 0 || choix == 0) break;

        if (choix == 1) {
            statsCalculer(0, 0, &resultat);
            uiSection("CHIFFRE D'AFFAIRES PAR SALLE");
            for (int i = 0; i < nb_salles; i++) {
                printf("  %-16s %10.2f TND\n", salles[i].nom,
                       resultat.chiffreAffairesParSalle[i]);
            }
            printf("  Total             %10.2f TND\n", resultat.chiffreAffaires);
        } else {
            int mois = uiLireEntier("Mois (1-12) : ", 1, 12);
            int annee = uiLireEntier("Annee : ", 1, 9999);
            if (mois < 0 || annee < 0) break;
            if (choix == 2) {
                statsCalculer(mois, annee, &resultat);
                printf("  Reservations enregistrees pour %02d/%04d : %d\n",
                       mois, annee, resultat.reservations);
            } else {
                afficherSallesPopulaires(mois, annee);
            }
        }
    } while (choix != 0);

    uiInfo("Retour au menu principal.");
}
