#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include "Facture.h"
#include "core/Persistence.h"


void genererFacture(Reservation res) {
    (void)persistenceGenererFacture(&res);
}
