#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "Core.h"
#include "Persistence.h"
#include "../FonctionsAux.h"

Salle salles[MAX_SALLES];
int nb_salles;
Reservation reservations[MAX_RES];
int nb_reservations;

Reservation *resTrouver(int id)
{
    for (int i = 0; i < nb_reservations; i++) {
        if (reservations[i].id == id) return &reservations[i];
    }
    return NULL;
}

static int validerHeure(const char *texte, int *minutes)
{
    int heure, minute;
    char reste;

    if (texte == NULL || strlen(texte) != 5 ||
        sscanf(texte, "%2d:%2d%c", &heure, &minute, &reste) != 2 ||
        texte[2] != ':' || heure < 0 || heure > 23 ||
        minute < 0 || minute > 59) {
        return 0;
    }
    *minutes = heure * 60 + minute;
    return 1;
}

static int validerDate(const char *texte)
{
    int jour, mois, annee;
    char reste;
    int joursMois;

    if (texte == NULL || strlen(texte) != 10 ||
        sscanf(texte, "%2d/%2d/%4d%c", &jour, &mois, &annee, &reste) != 3 ||
        texte[2] != '/' || texte[5] != '/' ||
        mois < 1 || mois > 12 || annee < 1) {
        return 0;
    }

    static const int jours[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    joursMois = jours[mois - 1];
    if (mois == 2 && (annee % 400 == 0 || (annee % 4 == 0 && annee % 100 != 0))) {
        joursMois = 29;
    }
    return jour >= 1 && jour <= joursMois;
}

static int champsValides(const char *client, int personnes, const char *date,
                         const char *debut, const char *fin, int *debutMinutes,
                         int *finMinutes, ResCode *erreur)
{
    if (client == NULL || client[0] == '\0' || strlen(client) >= sizeof(reservations[0].nom_client)) {
        *erreur = RES_NOM_INVALIDE;
        return 0;
    }
    if (personnes <= 0) {
        *erreur = RES_NOMBRE_INVALIDE;
        return 0;
    }
    if (!validerDate(date)) {
        *erreur = RES_DATE_INVALIDE;
        return 0;
    }
    if (!validerHeure(debut, debutMinutes) || !validerHeure(fin, finMinutes) ||
        *debutMinutes < 480 || *finMinutes > 1439 ||
        *finMinutes <= *debutMinutes) {
        *erreur = RES_HORAIRE_INVALIDE;
        return 0;
    }
    return 1;
}

static int reservationActive(const Reservation *reservation)
{
    return strcmp(reservation->statut, "annulee") != 0;
}

static int disponibiliteSalle(const Salle *salle, const char *date,
                              const char *debut, const char *fin, int ignorerId)
{
    for (int i = 0; i < nb_reservations; i++) {
        if (reservations[i].id == ignorerId || !reservationActive(&reservations[i])) {
            continue;
        }
        if (strcmp(reservations[i].salle.nom, salle->nom) == 0 &&
            strcmp(reservations[i].date, date) == 0 &&
            chevauche(reservations[i].heure_debut, reservations[i].heure_fin,
                      debut, fin)) {
            return 0;
        }
    }
    return 1;
}

static int trouverIndexReservation(int id)
{
    for (int i = 0; i < nb_reservations; i++) {
        if (reservations[i].id == id) {
            return i;
        }
    }
    return -1;
}

static int trouverIndexSalle(const char *nom)
{
    for (int i = 0; i < nb_salles; i++) {
        if (strcmp(salles[i].nom, nom) == 0) return i;
    }
    return -1;
}

static ResCode enregistrerChangement(void)
{
    return persistenceSauvegarderReservations() ? RES_OK : RES_ERREUR_FICHIER;
}

ResCode salleCreer(const char *nom, int capacite, float tarif,
                   const char *equipements)
{
    Salle nouvelle = {0};

    if (nom == NULL || equipements == NULL || nom[0] == '\0' ||
        strlen(nom) >= sizeof(nouvelle.nom) ||
        strlen(equipements) >= sizeof(nouvelle.equipements) ||
        capacite <= 0 || tarif < 0.0f) {
        return RES_ARGUMENT_INVALIDE;
    }
    if (nb_salles >= MAX_SALLES) return RES_CAPACITE_MAX;
    if (trouverIndexSalle(nom) >= 0) return RES_SALLE_EXISTANTE;

    strcpy(nouvelle.nom, nom);
    nouvelle.capacite = capacite;
    nouvelle.tarif_horaire = tarif;
    strcpy(nouvelle.equipements, equipements);
    salles[nb_salles++] = nouvelle;
    if (!persistenceSauvegarderSalles()) {
        nb_salles--;
        return RES_ERREUR_FICHIER;
    }
    return RES_OK;
}

ResCode salleModifier(const char *ancienNom, const char *nom, int capacite,
                      float tarif, const char *equipements)
{
    Salle nouvelle = {0};
    int index;
    Salle ancienne;
    Reservation anciennesReservations[MAX_RES];

    if (ancienNom == NULL || nom == NULL || equipements == NULL ||
        nom[0] == '\0' || strlen(nom) >= sizeof(nouvelle.nom) ||
        strlen(equipements) >= sizeof(nouvelle.equipements) ||
        capacite <= 0 || tarif < 0.0f) {
        return RES_ARGUMENT_INVALIDE;
    }
    index = trouverIndexSalle(ancienNom);
    if (index < 0) return RES_SALLE_INCONNUE;
    int indexNom = trouverIndexSalle(nom);
    if (indexNom >= 0 && indexNom != index) return RES_SALLE_EXISTANTE;

    ancienne = salles[index];
    memcpy(anciennesReservations, reservations,
           sizeof(Reservation) * (size_t)nb_reservations);
    strcpy(nouvelle.nom, nom);
    nouvelle.capacite = capacite;
    nouvelle.tarif_horaire = tarif;
    strcpy(nouvelle.equipements, equipements);
    salles[index] = nouvelle;

    int referencesModifiees = 0;
    for (int i = 0; i < nb_reservations; i++) {
        if (strcmp(reservations[i].salle.nom, ancienNom) == 0) {
            reservations[i].salle = nouvelle;
            referencesModifiees++;
        }
    }

    int ok = persistenceSauvegarderSalles();
    if (ok && referencesModifiees > 0) {
        ok = persistenceSauvegarderReservations();
    }
    for (int i = 0; ok && i < nb_reservations; i++) {
        if (strcmp(reservations[i].salle.nom, nom) == 0 &&
            strcmp(anciennesReservations[i].salle.nom, ancienNom) == 0) {
            ok = persistenceGenererFacture(&reservations[i]);
        }
    }
    if (!ok) {
        salles[index] = ancienne;
        memcpy(reservations, anciennesReservations,
               sizeof(Reservation) * (size_t)nb_reservations);
        (void)persistenceSauvegarderSalles();
        (void)persistenceSauvegarderReservations();
        for (int i = 0; i < nb_reservations; i++) {
            if (strcmp(anciennesReservations[i].salle.nom, ancienNom) == 0) {
                (void)persistenceGenererFacture(&anciennesReservations[i]);
            }
        }
        return RES_ERREUR_FICHIER;
    }
    return RES_OK;
}

ResCode salleSupprimer(const char *nom)
{
    int index;
    Salle ancienne[MAX_SALLES];
    int ancienNombre = nb_salles;

    if (nom == NULL) return RES_ARGUMENT_INVALIDE;
    index = trouverIndexSalle(nom);
    if (index < 0) return RES_SALLE_INCONNUE;

    memcpy(ancienne, salles, sizeof(Salle) * (size_t)nb_salles);
    for (int i = index; i < nb_salles - 1; i++) {
        salles[i] = salles[i + 1];
    }
    nb_salles--;
    if (!persistenceSauvegarderSalles()) {
        memcpy(salles, ancienne, sizeof(Salle) * (size_t)ancienNombre);
        nb_salles = ancienNombre;
        return RES_ERREUR_FICHIER;
    }
    return RES_OK;
}

ResCode coreChargerSalles(void)
{
    return persistenceChargerSalles() ? RES_OK : RES_ERREUR_FICHIER;
}

ResCode coreSauvegarderSalles(void)
{
    return persistenceSauvegarderSalles() ? RES_OK : RES_ERREUR_FICHIER;
}

ResCode coreInitialiserSalles(void)
{
    static const struct {
        const char *nom;
        int capacite;
        float tarif;
        const char *equipements;
    } defauts[] = {
        {"Mercure", 10, 10.0f, "Wi-Fi"},
        {"Venus", 15, 18.0f, "Projecteur, Wi-Fi"},
        {"Terre", 20, 25.0f, "Projecteur, Wi-Fi, Tableau blanc"},
        {"Mars", 12, 15.0f, "Wi-Fi, Tableau blanc"},
        {"Jupiter", 30, 30.0f, "Projecteur, Wi-Fi, Tableau blanc"},
        {"Saturne", 18, 20.0f, "Wi-Fi, Tableau blanc"},
        {"Uranus", 8, 12.0f, "Wi-Fi"},
        {"Neptune", 10, 15.0f, "Projecteur"}
    };

    ResCode code = coreChargerSalles();
    if (code != RES_OK || nb_salles > 0) return code;
    for (size_t i = 0; i < sizeof(defauts) / sizeof(defauts[0]); i++) {
        code = salleCreer(defauts[i].nom, defauts[i].capacite,
                          defauts[i].tarif, defauts[i].equipements);
        if (code != RES_OK) return code;
    }
    return RES_OK;
}

static ResCode creerOuModifier(int id, int modifier, const char *client,
                               int personnes, const char *date, const char *debut,
                               const char *fin, const char *nomSalle,
                               Reservation *resultat)
{
    int debutMinutes = 0, finMinutes = 0;
    ResCode erreur = RES_OK;
    int index = modifier ? trouverIndexReservation(id) : nb_reservations;
    int ancienNombre = nb_reservations;
    Salle *salle = NULL;
    Reservation ancienne = {0};
    Reservation nouvelle = {0};
    int maxId = 0;

    if (nomSalle == NULL || resultat == NULL) {
        return RES_ARGUMENT_INVALIDE;
    }
    if (modifier && index < 0) {
        return RES_INTROUVABLE;
    }
    if (!modifier && nb_reservations >= MAX_RES) {
        return RES_CAPACITE_MAX;
    }
    if (!champsValides(client, personnes, date, debut, fin,
                       &debutMinutes, &finMinutes, &erreur)) {
        return erreur;
    }

    for (int i = 0; i < nb_salles; i++) {
        if (strcmp(salles[i].nom, nomSalle) == 0) {
            salle = &salles[i];
            break;
        }
    }
    if (salle == NULL) {
        return RES_SALLE_INCONNUE;
    }
    if (personnes > salle->capacite) {
        return RES_CAPACITE;
    }
    if (!disponibiliteSalle(salle, date, debut, fin, modifier ? id : -1)) {
        return RES_CONFLIT;
    }

    if (!modifier) {
        for (int i = 0; i < nb_reservations; i++) {
            if (reservations[i].id > maxId) {
                maxId = reservations[i].id;
            }
        }
        if (maxId == INT_MAX) {
            return RES_CAPACITE_MAX;
        }
        nouvelle.id = maxId + 1;
    } else {
        ancienne = reservations[index];
        nouvelle.id = ancienne.id;
    }

    strcpy(nouvelle.nom_client, client);
    nouvelle.salle = *salle;
    strcpy(nouvelle.date, date);
    strcpy(nouvelle.heure_debut, debut);
    strcpy(nouvelle.heure_fin, fin);
    nouvelle.nombre_personnes = personnes;
    nouvelle.tarif = calculTarif(*salle, debut, fin);
    strcpy(nouvelle.statut, modifier ? "modifiee" : "confirmee");

    reservations[index] = nouvelle;
    if (!modifier) {
        nb_reservations++;
    }
    if (enregistrerChangement() != RES_OK ||
        !persistenceGenererFacture(&nouvelle)) {
        reservations[index] = ancienne;
        if (!modifier) {
            nb_reservations = ancienNombre;
        }
        (void)persistenceSauvegarderReservations();
        return RES_ERREUR_FICHIER;
    }
    *resultat = nouvelle;
    return RES_OK;
}

ResCode resCreer(const char *client, int personnes, const char *date,
                 const char *debut, const char *fin, const char *nomSalle,
                 Reservation *resultat)
{
    return creerOuModifier(0, 0, client, personnes, date, debut, fin,
                           nomSalle, resultat);
}

ResCode resModifier(int id, const char *client, int personnes, const char *date,
                    const char *debut, const char *fin, const char *nomSalle,
                    Reservation *resultat)
{
    if (id <= 0) {
        return RES_ARGUMENT_INVALIDE;
    }
    return creerOuModifier(id, 1, client, personnes, date, debut, fin,
                           nomSalle, resultat);
}

ResCode resAnnuler(int id)
{
    int index = trouverIndexReservation(id);
    Reservation ancienne;

    if (index < 0) {
        return RES_INTROUVABLE;
    }
    if (strcmp(reservations[index].statut, "annulee") == 0) {
        return RES_OK;
    }
    ancienne = reservations[index];
    strcpy(reservations[index].statut, "annulee");
    if (enregistrerChangement() != RES_OK) {
        reservations[index] = ancienne;
        return RES_ERREUR_FICHIER;
    }
    return RES_OK;
}

ResCode resSupprimer(int id)
{
    int index = trouverIndexReservation(id);
    Reservation ancienne[MAX_RES];
    int ancienNombre = nb_reservations;

    if (index < 0) {
        return RES_INTROUVABLE;
    }
    memcpy(ancienne, reservations, sizeof(Reservation) * (size_t)nb_reservations);
    for (int i = index; i < nb_reservations - 1; i++) {
        reservations[i] = reservations[i + 1];
    }
    nb_reservations--;
    if (enregistrerChangement() != RES_OK) {
        memcpy(reservations, ancienne, sizeof(Reservation) * (size_t)ancienNombre);
        nb_reservations = ancienNombre;
        return RES_ERREUR_FICHIER;
    }

    char nomFichier[64];
    snprintf(nomFichier, sizeof(nomFichier), "data/Facture_%d.txt", id);
    (void)remove(nomFichier);
    return RES_OK;
}

ResCode sallesDisponibles(int personnes, const char *date, const char *debut,
                          const char *fin, SallesDisponibles *resultat)
{
    int debutMinutes, finMinutes;
    ResCode erreur;

    if (resultat == NULL || personnes <= 0) {
        return RES_ARGUMENT_INVALIDE;
    }
    resultat->nombre = 0;
    if (!validerDate(date)) {
        return RES_DATE_INVALIDE;
    }
    if (!validerHeure(debut, &debutMinutes) || !validerHeure(fin, &finMinutes) ||
        debutMinutes < 480 || finMinutes > 1439 || finMinutes <= debutMinutes) {
        return RES_HORAIRE_INVALIDE;
    }
    for (int i = 0; i < nb_salles; i++) {
        if (salles[i].capacite >= personnes &&
            disponibiliteSalle(&salles[i], date, debut, fin, -1)) {
            resultat->elements[resultat->nombre++] = salles[i];
        }
    }
    erreur = resultat->nombre == 0 ? RES_CONFLIT : RES_OK;
    return erreur;
}

static void lireDateReservations(int index, int *jour, int *mois, int *annee)
{
    *jour = *mois = *annee = 0;
    (void)sscanf(reservations[index].date, "%2d/%2d/%4d", jour, mois, annee);
}

static int periodeInclut(int moisDonne, int anneeDonnee, int mois, int annee)
{
    return (moisDonne == 0 && anneeDonnee == 0) ||
           (moisDonne != 0 && anneeDonnee == 0 && moisDonne == mois) ||
           (moisDonne == 0 && anneeDonnee != 0 && anneeDonnee == annee) ||
           (moisDonne == mois && anneeDonnee == annee);
}

void statsCalculer(int mois, int annee, StatsResultat *resultat)
{
    if (resultat == NULL) {
        return;
    }
    memset(resultat, 0, sizeof(*resultat));
    resultat->mois = mois;
    resultat->annee = annee;

    int minJour = INT_MAX, maxJour = 0;
    int minMois = 13, minAnnee = INT_MAX, maxMois = 0, maxAnnee = 0;

    for (int i = 0; i < nb_reservations; i++) {
        int jour, moisRes, anneeRes;
        lireDateReservations(i, &jour, &moisRes, &anneeRes);
        if (jour == 0 || moisRes < 1 || moisRes > 12 || anneeRes < 1 ||
            !periodeInclut(mois, annee, moisRes, anneeRes)) {
            continue;
        }
        if (!reservationActive(&reservations[i])) {
            resultat->annulees++;
            continue;
        }

        resultat->reservations++;
        resultat->chiffreAffaires += reservations[i].tarif;
        resultat->reservationsParMois[moisRes - 1]++;
        resultat->heuresReservees +=
            (heureEnMinutes(reservations[i].heure_fin) -
             heureEnMinutes(reservations[i].heure_debut)) / 60.0;
        for (int salle = 0; salle < nb_salles; salle++) {
            if (strcmp(reservations[i].salle.nom, salles[salle].nom) == 0) {
                resultat->chiffreAffairesParSalle[salle] += reservations[i].tarif;
                resultat->reservationsParSalle[salle]++;
                break;
            }
        }
        if (anneeRes < minAnnee || (anneeRes == minAnnee && moisRes < minMois)) {
            minJour = jour;
            minMois = moisRes;
            minAnnee = anneeRes;
        }
        if (anneeRes > maxAnnee || (anneeRes == maxAnnee && moisRes > maxMois)) {
            maxJour = jour;
            maxMois = moisRes;
            maxAnnee = anneeRes;
        }
    }

    if (mois > 0 && annee > 0) {
        int joursDansMois = 31;
        if (mois == 4 || mois == 6 || mois == 9 || mois == 11) joursDansMois = 30;
        if (mois == 2) {
            joursDansMois = (annee % 400 == 0 || (annee % 4 == 0 && annee % 100 != 0)) ? 29 : 28;
        }
        resultat->heuresDisponibles = (double)nb_salles * joursDansMois * 16.0;
    } else if (mois == 0 && annee > 0) {
        int jours = (annee % 400 == 0 || (annee % 4 == 0 && annee % 100 != 0)) ? 366 : 365;
        resultat->heuresDisponibles = (double)nb_salles * jours * 16.0;
    } else if (minJour != INT_MAX && maxJour != 0) {
        int jours = 0;
        int anneeCourante = minAnnee, moisCourant = minMois, jourCourant = 1;
        while (anneeCourante < maxAnnee ||
               (anneeCourante == maxAnnee && moisCourant <= maxMois)) {
            int joursDansMois = 31;
            if (moisCourant == 4 || moisCourant == 6 || moisCourant == 9 || moisCourant == 11) joursDansMois = 30;
            if (moisCourant == 2) {
                joursDansMois = (anneeCourante % 400 == 0 ||
                    (anneeCourante % 4 == 0 && anneeCourante % 100 != 0)) ? 29 : 28;
            }
            if (anneeCourante == minAnnee && moisCourant == minMois) jourCourant = minJour;
            else jourCourant = 1;
            int dernierJour = (anneeCourante == maxAnnee && moisCourant == maxMois) ? maxJour : joursDansMois;
            jours += dernierJour - jourCourant + 1;
            moisCourant++;
            if (moisCourant > 12) {
                moisCourant = 1;
                anneeCourante++;
            }
        }
        resultat->heuresDisponibles = (double)nb_salles * jours * 16.0;
    }
}

ResCode coreChargerReservations(void)
{
    return persistenceChargerReservations() ? RES_OK : RES_ERREUR_FICHIER;
}

ResCode coreSauvegarderReservations(void)
{
    return persistenceSauvegarderReservations() ? RES_OK : RES_ERREUR_FICHIER;
}

const char *resCodeMessage(ResCode code)
{
    switch (code) {
        case RES_OK: return "Operation reussie.";
        case RES_SALLE_INCONNUE: return "Salle inconnue.";
        case RES_CAPACITE: return "La capacite de la salle est insuffisante.";
        case RES_CONFLIT: return "Aucune salle disponible pour ce creneau.";
        case RES_HORAIRE_INVALIDE: return "Horaire invalide : 08:00-23:59 et fin apres debut.";
        case RES_DATE_INVALIDE: return "Date invalide.";
        case RES_NOM_INVALIDE: return "Le nom du client est vide ou trop long.";
        case RES_NOMBRE_INVALIDE: return "Le nombre de personnes doit etre positif.";
        case RES_INTROUVABLE: return "Reservation introuvable.";
        case RES_CAPACITE_MAX: return "La capacite maximale est atteinte.";
        case RES_SALLE_OCCUPEE: return "Cette salle ne peut pas etre modifiee dans son etat actuel.";
        case RES_ERREUR_FICHIER: return "Erreur lors de l'acces aux fichiers de donnees.";
        case RES_ARGUMENT_INVALIDE: return "Les donnees fournies sont invalides.";
        case RES_SALLE_EXISTANTE: return "Une salle porte deja ce nom.";
    }
    return "Erreur inconnue.";
}
