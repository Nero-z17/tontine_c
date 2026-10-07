#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cycles.h"
#include "membres.h"
#include "utils.h"
#include "seances.h"
#include "cotisations.h"
#include "fichiers.h"
Cycle *nouveauCycle(void) {
    Cycle *c = calloc(1, sizeof(Cycle));
    if (c != NULL) c->etat = CYCLE_PLANIFIE;
    return c;
}
Cycle *trouverCycle(const Tontine *t, int idCycle) {
    Cycle *courant;
    if (t == NULL) return NULL;
    courant = t->cycles;
    while (courant != NULL) {
        if (courant->idCycle == idCycle) return courant;
        courant = courant->suivant;
    }
    return NULL;
}
int ajouterCycleFin(Tontine *t, Cycle *c) {
    Cycle *courant;
    if (t == NULL || c == NULL || trouverCycle(t, c->idCycle) != NULL) return 0;
    c->suivant = NULL;
    if (t->cycles == NULL) {
        t->cycles = c;
    } else {
        courant = t->cycles;
        while (courant->suivant != NULL) {
            courant = courant->suivant;
        }
        courant->suivant = c;
    }
    t->nombreCycles++;
    return 1;
}
void libererCycle(Cycle *c) {
    if (c == NULL) return;
    free(c->participants);
    libererFile(&c->fileBeneficiaires);
    free(c->seances);
    libererCotisations(c->cotisations);
    libererDettes(c->dettes);
    libererPaiements(c->paiements);
    free(c);
}
void libererTousLesCycles(Tontine *t) {
    Cycle *courant;
    Cycle *suivant;
    if (t == NULL) return;
    courant = t->cycles;
    while (courant != NULL) {
        suivant = courant->suivant;
        libererCycle(courant);
        courant = suivant;
    }
    t->cycles = NULL;
    t->nombreCycles = 0;
}
void initialiserFile(FileCirculaire *file) {
    file->tete = NULL;
    file->queue = NULL;
    file->taille = 0;
}
int enfiler(FileCirculaire *file, int idMembre) {
    NoeudFile *nouveau = malloc(sizeof(NoeudFile));
    if (nouveau == NULL) return 0;
    nouveau->idMembre = idMembre;
    if (file->tete == NULL) {
        file->tete = nouveau;
        file->queue = nouveau;
        nouveau->suivant = nouveau;
    } else {
        file->queue->suivant = nouveau;
        file->queue = nouveau;
        nouveau->suivant = file->tete;
    }
    file->taille++;
    return 1;
}
void libererFile(FileCirculaire *file) {
    NoeudFile *courant;
    NoeudFile *suivant;
    int i;
    courant = file->tete;
    for (i = 0; i < file->taille; i++) {
        suivant = courant->suivant;
        free(courant);
        courant = suivant;
    }
    initialiserFile(file);
}
void construireFile(Cycle *c) {
    int i;
    libererFile(&c->fileBeneficiaires);
    for (i = 0; i < c->nombreParticipants; i++) {
        enfiler(&c->fileBeneficiaires, c->participants[i]);
    }
}
int idBeneficiaireCourant(const Cycle *c) {
    if (c->fileBeneficiaires.tete == NULL) return -1;
    return c->fileBeneficiaires.tete->idMembre;
}
void avancerFile(Cycle *c) {
    FileCirculaire *file = &c->fileBeneficiaires;
    if (file->tete == NULL) return;
    file->tete = file->tete->suivant;
    file->queue = file->queue->suivant;
}
void positionnerFile(Cycle *c, int numeroSeance) {
    FileCirculaire *file = &c->fileBeneficiaires;
    int idCherche;
    int i;
    if (file->tete == NULL || numeroSeance < 1 || numeroSeance > c->nombreParticipants) return;
    idCherche = c->participants[numeroSeance - 1];
    for (i = 0; i < file->taille && file->tete->idMembre != idCherche; i++) {
        avancerFile(c);
    }
}
void tirerOrdreBeneficiaires(Cycle *c) {
    int i;
    int j;
    int temp;
    for (i = c->nombreParticipants - 1; i > 0; i--) {
        j = rand() % (i + 1);
        temp = c->participants[i];
        c->participants[i] = c->participants[j];
        c->participants[j] = temp;
    }
}
int demarrerCycle(Cycle *c) {
    if (c->etat != CYCLE_PLANIFIE) return 0;
    c->etat = CYCLE_EN_COURS;
    c->numeroSeanceActuelle = 1;
    if (ouvrirSeance(c, 1) == 0) {
        c->etat = CYCLE_PLANIFIE;
        c->numeroSeanceActuelle = 0;
        return 0;
    }
    return 1;
}
ResultatCreationCycle creerCycle(Tontine *t, int idCycle,
                                 const char *dateDebut,
                                 int frequenceJours,
                                 int montantCotisation,
                                 const int *idsParticipants,
                                 int nbParticipants) {
    Cycle *c;
    int i;
    int j;
    char message[TAILLE_MESSAGE_HISTORIQUE];
    if (trouverCycle(t, idCycle) != NULL) return CREATION_ID_EXISTE;
    if (idCycle <= 0 || montantCotisation <= 0 || frequenceJours <= 0 ||
        dateDebut == NULL) {
        return CREATION_PARAMETRES_INVALIDES;
    }
    if (nbParticipants < 2) return CREATION_PAS_ASSEZ_DE_PARTICIPANTS;
    if (idsParticipants == NULL) return CREATION_PARAMETRES_INVALIDES;
    for (i = 0; i < nbParticipants; i++) {
        if (!idMembreExiste(&t->membres, idsParticipants[i])) return CREATION_MEMBRE_INCONNU;
    }
    for (i = 0; i < nbParticipants; i++) {
        for (j = i + 1; j < nbParticipants; j++) {
            if (idsParticipants[i] == idsParticipants[j]) return CREATION_PARTICIPANT_EN_DOUBLE;
        }
    }
    c = nouveauCycle();
    if (c == NULL) return CREATION_ERREUR_MEMOIRE;
    c->idCycle = idCycle;
    strncpy(c->dateDebut, dateDebut, TAILLE_DATE - 1);
    c->dateDebut[TAILLE_DATE - 1] = '\0';
    c->frequenceJours = frequenceJours;
    c->montantCotisation = montantCotisation;
    c->nombreParticipants = nbParticipants;
    c->soldeCaisse = 0;
    c->participants = malloc(nbParticipants * sizeof(int));
    if (c->participants == NULL) {
        libererCycle(c);
        return CREATION_ERREUR_MEMOIRE;
    }
    for (i = 0; i < nbParticipants; i++) {
        c->participants[i] = idsParticipants[i];
    }
    tirerOrdreBeneficiaires(c);
    construireFile(c);
    if (initialiserSeances(c) == 0) {
        libererCycle(c);
        return CREATION_ERREUR_MEMOIRE;
    }
    if (demarrerCycle(c) == 0) {
        libererCycle(c);
        return CREATION_ERREUR_MEMOIRE;
    }
    if (ajouterCycleFin(t, c) == 0) {
        libererCycle(c);
        return CREATION_ID_EXISTE;
    }
    snprintf(message, sizeof(message),
             "Creation du cycle : %d participants, cotisation %d, ordre tire au sort",
             nbParticipants, montantCotisation);
    ajouterHistorique(idCycle, 0, message);
    return CREATION_OK;
}
const char *messageCreationCycle(ResultatCreationCycle resultat) {
    switch (resultat) {
        case CREATION_OK:
            return "Le cycle a ete cree avec succes.";
        case CREATION_ID_EXISTE:
            return "Erreur : un cycle avec cet identifiant existe deja.";
        case CREATION_PARAMETRES_INVALIDES:
            return "Erreur : parametres invalides (identifiant, montant, frequence ou date).";
        case CREATION_PAS_ASSEZ_DE_PARTICIPANTS:
            return "Erreur : il faut au moins 2 participants.";
        case CREATION_MEMBRE_INCONNU:
            return "Erreur : un des participants n'est pas un membre enregistre.";
        case CREATION_PARTICIPANT_EN_DOUBLE:
            return "Erreur : un meme membre est selectionne plusieurs fois.";
        case CREATION_ERREUR_MEMOIRE:
            return "Erreur : memoire insuffisante.";
    }
    return "Erreur inconnue.";
}
int membreParticipeAuCycle(const Cycle *c, int idMembre) {
    int i;
    if (c == NULL || c->participants == NULL) return 0;
    for (i = 0; i < c->nombreParticipants; i++) {
        if (c->participants[i] == idMembre) return 1;
    }
    return 0;
}
int membreAUnCycleNonTermine(const Tontine *t, int idMembre) {
    const Cycle *c;
    if (t == NULL) return 0;
    for (c = t->cycles; c != NULL; c = c->suivant) {
        if (c->etat != CYCLE_TERMINE && membreParticipeAuCycle(c, idMembre)) return 1;
    }
    return 0;
}
int cycleAccepteOperations(const Cycle *c) {
    return c != NULL && c->etat == CYCLE_EN_COURS;
}
int terminerCycle(Cycle *c) {
    if (c == NULL || c->etat == CYCLE_TERMINE) return 0;
    c->etat = CYCLE_TERMINE;
    ajouterHistorique(c->idCycle, c->numeroSeanceActuelle,
                      "Fin du cycle : toutes les seances sont cloturees");
    return 1;
}
Cycle *choisirCycle(const Tontine *t, int exigerEnCours) {
    Cycle *c;
    int id;
    if (t == NULL || t->cycles == NULL) {
        printf("Aucun cycle n'est enregistre.\n");
        return NULL;
    }
    afficherTousLesCycles(t);
    printf("\nIdentifiant du cycle : ");
    scanf("%d", &id);
    c = trouverCycle(t, id);
    if (c == NULL) {
        printf("Aucun cycle ne correspond a cet identifiant.\n");
        return NULL;
    }
    if (exigerEnCours && !cycleAccepteOperations(c)) {
        printf("Ce cycle n'est pas en cours (etat : %s).\n",
               libelleEtatCycle(c->etat));
        return NULL;
    }
    return c;
}
void afficherCycle(const Cycle *c) {
    if (c == NULL) return;
    afficherTitre("INFORMATIONS DU CYCLE");
    printf("Cycle               : %d\n", c->idCycle);
    printf("Etat                : %s\n", libelleEtatCycle(c->etat));
    printf("Date de debut       : %s\n", c->dateDebut);
    printf("Frequence           : tous les %d jours\n", c->frequenceJours);
    printf("Cotisation          : %d FCFA\n", c->montantCotisation);
    printf("Participants        : %d\n", c->nombreParticipants);
    printf("Seance actuelle     : %d / %d\n",
           c->numeroSeanceActuelle, c->nombreParticipants);
    printf("Solde de la caisse  : %d FCFA\n", c->soldeCaisse);
}
void afficherTousLesCycles(const Tontine *t) {
    const Cycle *c;
    if (t == NULL || t->cycles == NULL) {
        printf("Aucun cycle n'est enregistre.\n");
        return;
    }
    afficherTitre("LISTE DES CYCLES");
    printf("%-5s %-10s %-11s %-9s %-11s %-13s %s\n",
           "ID", "Etat", "Debut", "Freq.(j)", "Cotisation", "Participants", "Seance");
    for (c = t->cycles; c != NULL; c = c->suivant) {
        printf("%-5d %-10s %-11s %-9d %-11d %-13d %d/%d\n",
               c->idCycle, libelleEtatCycle(c->etat), c->dateDebut,
               c->frequenceJours, c->montantCotisation, c->nombreParticipants,
               c->numeroSeanceActuelle, c->nombreParticipants);
    }
}
void afficherParticipants(const Tontine *t, const Cycle *c) {
    int i;
    int id;
    if (c == NULL || c->participants == NULL) return;
    printf("\nParticipants du cycle %d :\n", c->idCycle);
    for (i = 0; i < c->nombreParticipants; i++) {
        id = c->participants[i];
        printf("  ID %d - %s\n", id,
               t != NULL ? nomDuMembre(&t->membres, id) : "?");
    }
}
void afficherOrdreBeneficiaires(const Tontine *t, const Cycle *c) {
    const Seance *s;
    int k;
    int id;
    if (c == NULL || c->participants == NULL) return;
    printf("\nOrdre des beneficiaires du cycle %d :\n", c->idCycle);
    for (k = 1; k <= c->nombreParticipants; k++) {
        id = c->participants[k - 1];
        s = trouverSeance(c, k);
        printf("  Seance %d", k);
        if (s != NULL) printf(" (%s)", s->date);
        printf(" -> ID %d - %s", id,
               t != NULL ? nomDuMembre(&t->membres, id) : "?");
        if (s != NULL && s->etat == SEANCE_CLOTUREE) printf("   [cloturee]");
        else if (c->etat == CYCLE_EN_COURS && k == c->numeroSeanceActuelle) {
            printf("   <-- seance actuelle");
        }
        printf("\n");
    }
}
void afficherBilanCycle(const Tontine *t, const Cycle *c) {
    const Dette *d;
    char titre[64];
    int dettesEnCours = 0;
    int penalitesCalculees;
    int penalitesPayees;
    if (c == NULL) return;
    snprintf(titre, sizeof(titre), "BILAN DU CYCLE %d", c->idCycle);
    afficherTitre(titre);
    if (c->etat != CYCLE_TERMINE) printf("(Bilan provisoire : le cycle n'est pas termine.)\n");
    afficherCycle(c);
    afficherParticipants(t, c);
    afficherOrdreBeneficiaires(t, c);
    afficherHistoriqueSeances(t, c);
    for (d = c->dettes; d != NULL; d = d->suivant) {
        if (d->etat != DETTE_REGULARISEE) dettesEnCours++;
    }
    penalitesCalculees = totalPenalitesCalculees(c);
    penalitesPayees = totalPenalitesPayees(c);
    afficherTitre("RESUME FINANCIER");
    printf("Cotisations attendues      : %d FCFA\n", totalCotisationsAttendues(c));
    printf("Total encaisse             : %d FCFA\n", totalEncaisse(c));
    printf("Dettes non regularisees    : %d\n", dettesEnCours);
    printf("Principal restant a payer  : %d FCFA\n", totalPrincipalRestant(c));
    printf("Penalites calculees        : %d FCFA\n", penalitesCalculees);
    printf("Penalites payees           : %d FCFA\n", penalitesPayees);
    printf("Penalites restant a payer  : %d FCFA\n", penalitesCalculees - penalitesPayees);
    printf("Solde final de la caisse   : %d FCFA\n", c->soldeCaisse);
}
static int participantDejaChoisi(const int *ids, int nb, int id) {
    int i;
    for (i = 0; i < nb; i++) {
        if (ids[i] == id) return 1;
    }
    return 0;
}
static void saisirEtCreerCycle(Tontine *t) {
    char date[TAILLE_DATE];
    char invite[80];
    int *ids;
    int idCycle;
    int frequence;
    int montant;
    int nb;
    int i;
    int id;
    ResultatCreationCycle resultat;
    if (t->membres.taille < 2) {
        printf("Il faut au moins 2 membres enregistres pour creer un cycle.\n");
        return;
    }
    afficherTitre("CREATION D'UN CYCLE");
    printf("Identifiant du cycle : ");
    scanf("%d", &idCycle);
    if (trouverCycle(t, idCycle) != NULL) {
        printf("%s\n", messageCreationCycle(CREATION_ID_EXISTE));
        return;
    }
    printf("Date de debut (JJ/MM/AAAA) : ");
    scanf("%10s", date);
    printf("Frequence des seances en jours (7, 14, 30...) : ");
    scanf("%d", &frequence);
    printf("Montant de la cotisation (FCFA) : ");
    scanf("%d", &montant);
    afficherTousLesMembres(&t->membres);
    printf("\nNombre de participants : ");
    scanf("%d", &nb);
    ids = malloc(nb * sizeof(int));
    if (ids == NULL) {
        printf("%s\n", messageCreationCycle(CREATION_ERREUR_MEMOIRE));
        return;
    }
    i = 0;
    while (i < nb) {
        snprintf(invite, sizeof(invite),
                 "Participant %d/%d - identifiant du membre : ", i + 1, nb);
        printf("%s", invite);
        scanf("%d", &id);
        if (!idMembreExiste(&t->membres, id)) printf("Ce membre n'existe pas.\n");
        else if (participantDejaChoisi(ids, i, id)) {
            printf("Ce membre est deja selectionne.\n");
        } else {
            ids[i] = id;
            i++;
        }
    }
    printf("\nRecapitulatif : cycle %d, debut %s, une seance tous les %d jours,\n"
           "cotisation de %d FCFA, %d participants.\n",
           idCycle, date, frequence, montant, nb);
    printf("L'ordre des beneficiaires sera tire au sort et ne changera plus.\n");
    {
        char confirmation[4];
        printf("Confirmer la creation du cycle ? (o/n) : ");
        scanf("%3s", confirmation);
        if (confirmation[0] != 'o' && confirmation[0] != 'O') {
            printf("Creation annulee.\n");
            free(ids);
            return;
        }
    }
    resultat = creerCycle(t, idCycle, date, frequence, montant, ids, nb);
    free(ids);
    printf("%s\n", messageCreationCycle(resultat));
    if (resultat == CREATION_OK) {
        afficherOrdreBeneficiaires(t, trouverCycle(t, idCycle));
        if (!sauvegarderTout(t)) printf("Erreur : impossible de sauvegarder les donnees.\n");
    }
}
void menuCycles(Tontine *t) {
    Cycle *c;
    int choix;
    if (t == NULL) return;
    do {
        afficherTitre("GESTION DES CYCLES");
        printf("1. Creer un cycle\n");
        printf("2. Afficher les cycles\n");
        printf("3. Afficher les participants d'un cycle\n");
        printf("4. Afficher l'ordre des beneficiaires\n");
        printf("5. Consulter un cycle\n");
        printf("6. Consulter le bilan d'un cycle\n");
        printf("0. Retour\n");
        printf("\nVotre choix : ");
        scanf("%d", &choix);
        switch (choix) {
            case 1:
                saisirEtCreerCycle(t);
                break;
            case 2:
                afficherTousLesCycles(t);
                break;
            case 3:
                c = choisirCycle(t, 0);
                if (c != NULL) afficherParticipants(t, c);
                break;
            case 4:
                c = choisirCycle(t, 0);
                if (c != NULL) afficherOrdreBeneficiaires(t, c);
                break;
            case 5:
                c = choisirCycle(t, 0);
                if (c != NULL) afficherCycle(c);
                break;
            case 6:
                c = choisirCycle(t, 0);
                if (c != NULL) afficherBilanCycle(t, c);
                break;
            case 0:
                break;
        }
    } while (choix != 0);
}
void menuBilans(Tontine *t) {
    Cycle *c;
    if (t == NULL) return;
    c = choisirCycle(t, 0);
    if (c != NULL) afficherBilanCycle(t, c);
}
