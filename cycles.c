#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cycles.h"
#include "membres.h"
#include "utils.h"
#include "seances.h"
#include "cotisations.h"
#include "fichiers.h"

Cycle *nouveauCycle(void)
{
    Cycle *c = calloc(1, sizeof(Cycle));    /* tout à 0 et NULL */
    if (c != NULL)
    {
        c->etat = CYCLE_PLANIFIE;
    }
    return c;
}

int ajouterCycleFin(Tontine *t, Cycle *c)
{
    Cycle *courant;

    if (trouverCycle(t, c->idCycle) != NULL)
    {
        return 0;                           /* identifiant déjà pris */
    }

    c->suivant = NULL;
    if (t->cycles == NULL)
    {
        t->cycles = c;                      /* premier cycle */
    }
    else
    {
        courant = t->cycles;
        while (courant->suivant != NULL)    /* on va jusqu'au dernier */
        {
            courant = courant->suivant;
        }
        courant->suivant = c;
    }
    t->nombreCycles++;
    return 1;
}

Cycle *trouverCycle(const Tontine *t, int idCycle)
{
    Cycle *courant = t->cycles;

    while (courant != NULL)
    {
        if (courant->idCycle == idCycle)
        {
            return courant;
        }
        courant = courant->suivant;
    }
    return NULL;
}

void libererCycle(Cycle *c)
{
    if (c == NULL)
    {
        return;
    }
    free(c->participants);
    libererFile(&c->fileBeneficiaires);
    free(c->seances);
    libererCotisations(c->cotisations);
    libererDettes(c->dettes);
    libererPaiements(c->paiements);
    free(c);
}

void initialiserFile(FileCirculaire *file){
    file->tete = NULL;
    file->queue = NULL;
    file->taille = 0;
}

int enfiler(FileCirculaire *file, int idMembre){
    NoeudFile *n = malloc(sizeof(*n));
    if(n == NULL) return 0;
    n->idMembre = idMembre;
    n->suivant = NULL;
    if(file->queue == NULL){
        file->tete = n;
        file->queue = n;
        file->queue->suivant = file->tete;
    }else{
        file->queue->suivant = n;
        file->queue = n;
        file->queue->suivant = file->tete;
    }
    file->taille++;
    return 1;
}

void libererFile(FileCirculaire *file){
    NoeudFile *courant;
    NoeudFile *suivant;
    int i;
    courant = file->tete;
    for (i = 0; i < file->taille; i++){      /* on compte : la liste ne finit jamais */
        suivant = courant->suivant;
        free(courant);
        courant = suivant;
    }
    initialiserFile(file);
}

void construireFile(Cycle *c){
    int i;
    libererFile(&c->fileBeneficiaires);     /* 1. on vide l'ancienne file */
    for (i = 0; i < c->nombreParticipants; i++){
        enfiler(&c->fileBeneficiaires, c->participants[i]);   /* 2. dans l'ordre */
    }
}
void tirerOrdreBeneficiaires(Cycle *c){
    int i;
    int j;
    int temp;

    for (i = c->nombreParticipants - 1; i > 0; i--){
        j = rand() % (i + 1);               /* position au hasard entre 0 et i */

        temp = c->participants[i];          /* on échange les cases i et j */
        c->participants[i] = c->participants[j];
        c->participants[j] = temp;
    }
}

int idBeneficiaireCourant(const Cycle *c){
    if(c->fileBeneficiaires.tete == NULL){
        return -1;                          /* file vide */
    }
    return c->fileBeneficiaires.tete->idMembre;
}

void avancerFile(Cycle *c){
    FileCirculaire *file = &c->fileBeneficiaires;
    if (file->tete == NULL){
        return;
    }
    file->tete = file->tete->suivant;       /* le suivant devient la tête */
    file->queue = file->queue->suivant;     /* l'ancienne tête devient la queue */
}

void positionnerFile(Cycle *c, int numeroSeance){
    FileCirculaire *file = &c->fileBeneficiaires;
    int idCherche;
    int i;
    if (file->tete == NULL || numeroSeance < 1 || numeroSeance > c->nombreParticipants){
        return;                             /* rien à faire */
    }
    idCherche = c->participants[numeroSeance - 1];
    for (i = 0; i < file->taille && file->tete->idMembre != idCherche; i++){
        avancerFile(c);
    }
}

ResultatCreationCycle creerCycle(Tontine *t, int idCycle,
                                 const char *dateDebut,
                                 int frequenceJours,
                                 int montantCotisation,
                                 const int *idsParticipants,
                                 int nbParticipants)
{
    Cycle *c;
    int i;
    int j;
    char message[TAILLE_MESSAGE_HISTORIQUE];
    /* ---------- 1. Vérifications (rien n'est encore alloué) ---------- */
    if (trouverCycle(t, idCycle) != NULL){
        return CREATION_ID_EXISTE;
    }
    if (idCycle <= 0 || montantCotisation <= 0 || frequenceJours <= 0 ||
        dateDebut == NULL || !dateEstValide(dateDebut)){
        return CREATION_PARAMETRES_INVALIDES;
    }
    if (nbParticipants < 2){
        return CREATION_PAS_ASSEZ_DE_PARTICIPANTS;
    }

    if (idsParticipants == NULL)
    {
        return CREATION_PARAMETRES_INVALIDES;
    }

    for (i = 0; i < nbParticipants; i++)    /* chaque participant est un membre ? */
    {
        if (!idMembreExiste(&t->membres, idsParticipants[i]))
        {
            return CREATION_MEMBRE_INCONNU;
        }
    }

    for (i = 0; i < nbParticipants; i++)    /* aucun participant en double ? */
    {
        for (j = i + 1; j < nbParticipants; j++)
        {
            if (idsParticipants[i] == idsParticipants[j])
            {
                return CREATION_PARTICIPANT_EN_DOUBLE;
            }
        }
    }

    /* ---------- 2. Création du cycle et champs généraux ---------- */
    c = nouveauCycle();
    if (c == NULL)
    {
        return CREATION_ERREUR_MEMOIRE;
    }

    c->idCycle = idCycle;
    strncpy(c->dateDebut, dateDebut, TAILLE_DATE - 1);
    c->dateDebut[TAILLE_DATE - 1] = '\0';
    c->frequenceJours = frequenceJours;
    c->montantCotisation = montantCotisation;
    c->nombreParticipants = nbParticipants;
    c->soldeCaisse = 0;

    /* ---------- 3. Copie des participants dans le tableau ---------- */
    c->participants = malloc(nbParticipants * sizeof(int));
    if (c->participants == NULL)
    {
        libererCycle(c);
        return CREATION_ERREUR_MEMOIRE;
    }
    for (i = 0; i < nbParticipants; i++)
    {
        c->participants[i] = idsParticipants[i];
    }

    /* ---------- 4 et 5. Tirage au sort, puis file circulaire ---------- */
    tirerOrdreBeneficiaires(c);
    construireFile(c);

    /* ---------- 6. Les séances ---------- */
    if (initialiserSeances(c) == 0)
    {
        libererCycle(c);
        return CREATION_ERREUR_MEMOIRE;
    }

    /* ---------- 7. Démarrage : séance 1 ouverte ---------- */
    if (demarrerCycle(c) == 0)
    {
        libererCycle(c);
        return CREATION_ERREUR_MEMOIRE;
    }

    /* ---------- 8. Le cycle rejoint la tontine (en dernier !) ---------- */
    if (ajouterCycleFin(t, c) == 0)
    {
        libererCycle(c);
        return CREATION_ID_EXISTE;
    }

    snprintf(message, sizeof(message),
             "Creation du cycle : %d participants, cotisation %d, ordre tire au sort",
             nbParticipants, montantCotisation);
    ajouterHistorique(idCycle, 0, message);

    return CREATION_OK;
}

int demarrerCycle(Cycle *c)
{
    if (c->etat != CYCLE_PLANIFIE)
    {
        return 0;                           /* déjà démarré ou terminé */
    }

    c->etat = CYCLE_EN_COURS;
    c->numeroSeanceActuelle = 1;

    if (ouvrirSeance(c, 1) == 0)
    {
        c->etat = CYCLE_PLANIFIE;           /* échec : on annule */
        c->numeroSeanceActuelle = 0;
        return 0;
    }
    return 1;
}

const char *messageCreationCycle(ResultatCreationCycle resultat)
{
    switch (resultat)
    {
        case CREATION_OK:
            return "Le cycle a été créé avec succès.";
        case CREATION_ID_EXISTE:
            return "Erreur : un cycle avec cet identifiant existe déjà.";
        case CREATION_PARAMETRES_INVALIDES:
            return "Erreur : paramètres invalides (identifiant, montant, fréquence ou date).";
        case CREATION_PAS_ASSEZ_DE_PARTICIPANTS:
            return "Erreur : il faut au moins 2 participants.";
        case CREATION_MEMBRE_INCONNU:
            return "Erreur : un des participants n'est pas un membre enregistré.";
        case CREATION_PARTICIPANT_EN_DOUBLE:
            return "Erreur : un même membre est sélectionné plusieurs fois.";
        case CREATION_ERREUR_MEMOIRE:
            return "Erreur : mémoire insuffisante.";
    }
    return "Erreur inconnue.";
}