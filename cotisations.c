#include<stdio.h>
#include<stdlib.h>
#include<cotisations.h>


Cotisation *nouvelleCotisation(void){
    Cotisation *coti = malloc(sizeof *coti);
    if(coti == NULL) return NULL;
    coti->idCycle = 0;
    coti->numeroSeance = 0;
    coti->idMembre = 0;
    coti->montantAttendu = 0;
    coti->montantPaye = 0;
    coti->etat = COTISATION_NON_PAYEE;
    coti->suivant = NULL;
    return coti;
}

Dette *nouvelleDette(void){
    Dette *dette = malloc(sizeof *dette);
    if(dette == NULL) return NULL;
    idCycle;
    int numeroSeanceOrigine;
    int idDebiteur;
    int idBeneficiaireOrigine;

    int montantPayeInitial;         /* payé lors de la séance d'origine */
    int principalInitial;           /* manque initial (attendu - payé) */
    int principalRestant;           /* principal encore dû */

    int nombreSeancesRetard;
    TypePenalite typePenalite;
    int penaliteTotale;             /* pénalités calculées */
    int penalitePayee;              /* pénalités réellement payées */

    EtatDette etat;
    int numeroSeanceRegularisation; /* 0 tant que non régularisée */
    struct Dette *suivant;
}