#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FonctionsAux.h"

// ---------------------------
// Fonctions auxiliaires
// ---------------------------

// Convertit une heure "HH:MM" en minutes totales
int heureEnMinutes(const char heure[6]) {
    int h, m;
    sscanf(heure, "%d:%d", &h, &m);
    return h*60 + m;
}

// Vérifie si deux créneaux horaires se chevauchent
int chevauche(const char debut1[6], const char fin1[6],
              const char debut2[6], const char fin2[6]) {
    int d1 = heureEnMinutes(debut1);
    int f1 = heureEnMinutes(fin1);
    int d2 = heureEnMinutes(debut2);
    int f2 = heureEnMinutes(fin2);
    return (d1 < f2) && (d2 < f1);
}

// Calcule le tarif d'une réservation
float calculTarif(Salle salle, const char debut[6], const char fin[6]) {
    int minutes = heureEnMinutes(fin) - heureEnMinutes(debut);
    return (minutes / 60.0f) * salle.tarif_horaire;
}

// Vérifie si une salle est disponible pour une date et un créneau
int salleDisponible(Salle salle, const char date[11],
                    const char debut[6], const char fin[6]) {
    for(int i=0; i<nb_reservations; i++) {
        if(strcmp(reservations[i].salle.nom, salle.nom)==0 &&
           strcmp(reservations[i].date, date)==0 &&
           strcmp(reservations[i].statut, "annulee") != 0 &&
           chevauche(reservations[i].heure_debut, reservations[i].heure_fin, debut, fin)) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------
// Nettoyage des chaînes
// ---------------------------
void nettoyerChaine(char *str) {
    for(int i=0; str[i]!='\0'; i++){
        if(str[i]=='\n'){ str[i]='\0'; break; }
    }
}

// ---------------------------
// Lecture sécurisée de la date et de l'heure
// ---------------------------
void lireDate(char date[11]) {
    char buffer[20];
    int j,m,a;
    do {
        printf("Date (JJ/MM/AAAA) : ");
        fgets(buffer,sizeof(buffer),stdin);
        if(sscanf(buffer,"%d/%d/%d",&j,&m,&a)==3 && j>=1 && j<=31 && m>=1 && m<=12 && a>=2023){
            sprintf(date,"%02d/%02d/%04d",j,m,a);
            break;
        } else {
            printf("Format invalide. Reessayez.\n");
        }
    } while(1);
}

void lireHeure(char heure[6], const char *message) {
    char buffer[20];
    int h,m;
    do {
        printf("%s (HH:MM) : ", message);
        fgets(buffer,sizeof(buffer),stdin);
        if(sscanf(buffer,"%d:%d",&h,&m)==2 && h>=0 && h<=23 && m>=0 && m<=59){
            sprintf(heure,"%02d:%02d",h,m);
            break;
        } else {
            printf("Format invalide. Reessayez.\n");
        }
    } while(1);
}