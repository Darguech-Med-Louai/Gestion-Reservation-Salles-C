#ifndef STRUCTURES_H
#define STRUCTURES_H

#define MAX_SALLES 20   // Nombre maximum de salles
#define MAX_RES 100     // Nombre maximum de réservations

// ---------------------------
// Structure pour une salle
// ---------------------------
typedef struct {
    char nom[50];         
    int capacite;         
    float tarif_horaire;  
    char equipements[200];
} Salle;

// ---------------------------
// Structure pour une réservation
// ---------------------------
typedef struct {
    int id;               
    char nom_client[50];  
    Salle salle;          
    char date[11];        
    char heure_debut[6];  
    char heure_fin[6];    
    int nombre_personnes; 
    float tarif;          
    char statut[20];      
} Reservation;

extern Salle salles[MAX_SALLES];
extern int nb_salles;

extern Reservation reservations[MAX_RES];
extern int nb_reservations;
#endif // STRUCTURES_H
