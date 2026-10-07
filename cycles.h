#ifndef CYCLES_H
#define CYCLES_H

/* ============================================================
   CYCLES.H - Cycles, file circulaire, tirage, bilan
   Responsable : Personne 2
   Fichiers    : cycles.h / cycles.c

   Ce module appelle :
     - membres.h     : trouverMembreParId(), nomDuMembre(), existeAuMoinsUnMembre()
     - seances.h     : initialiserSeances(), ouvrirSeance(), afficherSeance()
     - cotisations.h : totaux pour le bilan (totalEncaisse, ...)
     - fichiers.h    : sauvegarderTout(), ajouterHistorique()
     - utils.h       : affichage, dates

   Ce module est appelé par :
     - membres.c     : membreAUnCycleNonTermine()
     - seances.c     : avancerFile(), terminerCycle(), choisirCycle(), ...
     - cotisations.c : membreParticipeAuCycle(), choisirCycle()
     - fichiers.c    : nouveauCycle(), ajouterCycleFin(), construireFile(),
                       positionnerFile(), libererTousLesCycles()
     - main.c        : menuCycles(), menuBilans()

   Rappel : un cycle contient TOUTES ses données (participants,
   file, séances, cotisations, dettes, paiements, caisse).
   Les fonctions marquées [PRIMITIVE] sont à écrire EN PREMIER.
   ============================================================ */

#include "structures.h"


/* Résultat de la création d'un cycle */
typedef enum{
    CREATION_OK,
    CREATION_ID_EXISTE,
    CREATION_PARAMETRES_INVALIDES,       /* montant <= 0, frequence <= 0, date fausse */
    CREATION_PAS_ASSEZ_DE_PARTICIPANTS,  /* il faut au moins 2 participants */
    CREATION_MEMBRE_INCONNU,
    CREATION_PARTICIPANT_EN_DOUBLE,
    CREATION_ERREUR_MEMOIRE
} ResultatCreationCycle;


/* ============================================================
   1. PRIMITIVES SUR LES CYCLES
   ============================================================ */

/*
 * [PRIMITIVE] Alloue un cycle VIDE (calloc) : tous les champs à 0 /
 * NULL, etat = CYCLE_PLANIFIE. Retourne NULL si l'allocation échoue.
 */
Cycle *nouveauCycle(void);

/*
 * [PRIMITIVE] Ajoute le cycle EN FIN de la liste t->cycles et
 * incrémente t->nombreCycles. Retourne 0 si l'identifiant existe
 * déjà, 1 sinon.
 */
int ajouterCycleFin(Tontine *t, Cycle *c);

/* [PRIMITIVE] Retourne le cycle d'identifiant idCycle, ou NULL. */
Cycle *trouverCycle(const Tontine *t, int idCycle);

/*
 * [PRIMITIVE] Libère UN cycle et tout ce qu'il contient :
 * participants, file (libererFile), séances (tableau),
 * cotisations, dettes, paiements (libererCotisations/Dettes/
 * Paiements du module cotisations), puis le cycle lui-même.
 */
void libererCycle(Cycle *c);

/* [PRIMITIVE] Libère tous les cycles de la tontine. */
void libererTousLesCycles(Tontine *t);


/* ============================================================
   2. FILE CIRCULAIRE DES BENEFICIAIRES
   ============================================================ */

/* [PRIMITIVE] File vide : tete = queue = NULL, taille = 0. */
void initialiserFile(FileCirculaire *file);

/*
 * [PRIMITIVE] Ajoute idMembre à la FIN de la file et garde la
 * circularité (queue->suivant == tete). Retourne 1 si OK, 0 sinon.
 */
int enfiler(FileCirculaire *file, int idMembre);

/*
 * [PRIMITIVE] Construit c->fileBeneficiaires à partir du tableau
 * c->participants (dans l'ordre du tableau). Si la file contient
 * déjà des noeuds, elle est d'abord libérée.
 */
void construireFile(Cycle *c);

/*
 * [PRIMITIVE] Place la tête de la file sur le bénéficiaire de la
 * séance numeroSeance (1 = premier de l'ordre). Sert au chargement
 * des fichiers : construireFile(c) puis
 * positionnerFile(c, c->numeroSeanceActuelle).
 */
void positionnerFile(Cycle *c, int numeroSeance);

/* Retourne l'idMembre en tête de file (bénéficiaire courant), ou -1. */
int idBeneficiaireCourant(const Cycle *c);

/*
 * Passe au bénéficiaire suivant SANS retirer de noeud :
 *     tete = tete->suivant;  queue = queue->suivant;
 * (l'ordre n'est jamais perdu). Appelée par seances.c.
 */
void avancerFile(Cycle *c);

/* [PRIMITIVE] Libère les noeuds (attention : file circulaire !). */
void libererFile(FileCirculaire *file);


/* ============================================================
   3. CREATION D'UN CYCLE (cahier §6 et §7)
   ============================================================ */

/*
 * Mélange le tableau c->participants (tirage au sort, algorithme
 * de Fisher-Yates). À appeler UNE SEULE FOIS, à la création :
 * l'ordre ne change plus jamais (Règle 10).
 */
void tirerOrdreBeneficiaires(Cycle *c);

/*
 * Crée un cycle complet. Étapes dans CET ordre :
 *   1. vérifier les paramètres (id libre, montant > 0, fréquence > 0,
 *      date valide, au moins 2 participants, membres existants,
 *      pas de doublon)
 *   2. nouveauCycle() puis remplir les champs généraux
 *      (nombreParticipants = nbParticipants, soldeCaisse = 0)
 *   3. copier idsParticipants dans c->participants (tableau)
 *   4. tirerOrdreBeneficiaires(c)
 *   5. construireFile(c)
 *   6. initialiserSeances(c)            (module seances)
 *   7. ajouterCycleFin(t, c)
 *   8. demarrerCycle(c)
 * En cas d'erreur après une allocation : libérer ce qui a été alloué.
 */
ResultatCreationCycle creerCycle(Tontine *t, int idCycle,
                                 const char *dateDebut,
                                 int frequenceJours,
                                 int montantCotisation,
                                 const int *idsParticipants,
                                 int nbParticipants);

/* Texte français expliquant le résultat (à afficher). */
const char *messageCreationCycle(ResultatCreationCycle resultat);

/*
 * Passe le cycle de PLANIFIE à EN_COURS : numeroSeanceActuelle = 1
 * puis ouvrirSeance(c, 1) (module seances).
 * Retourne 1 si OK, 0 si le cycle n'était pas PLANIFIE.
 */
int demarrerCycle(Cycle *c);


/* ============================================================
   4. ETAT D'UN CYCLE
   ============================================================ */

/* Retourne 1 si le membre figure dans c->participants. */
int membreParticipeAuCycle(const Cycle *c, int idMembre);

/*
 * Retourne 1 si le membre participe à AU MOINS UN cycle dont
 * l'état n'est pas CYCLE_TERMINE. Sert à refuser la suppression
 * d'un membre (cahier §4.4).
 */
int membreAUnCycleNonTermine(const Tontine *t, int idMembre);

/* Retourne 1 si etat == CYCLE_EN_COURS (opérations autorisées). */
int cycleAccepteOperations(const Cycle *c);

/*
 * Passe le cycle à CYCLE_TERMINE (appelée par seances.c après la
 * clôture de la dernière séance). Tout est conservé.
 * Retourne 1 si OK, 0 si le cycle était déjà terminé.
 */
int terminerCycle(Cycle *c);

/*
 * Demande un identifiant de cycle à l'utilisateur et retourne le
 * cycle. Affiche elle-même le message d'erreur et retourne NULL si
 * le cycle n'existe pas, ou (si exigerEnCours == 1) s'il n'est pas
 * EN_COURS. Utilisée par tous les menus qui travaillent sur un cycle.
 */
Cycle *choisirCycle(const Tontine *t, int exigerEnCours);


/* ============================================================
   5. AFFICHAGE
   ============================================================ */

/* Informations générales d'un cycle (cahier §30, "Informations générales"). */
void afficherCycle(const Cycle *c);

/* Liste résumée de tous les cycles. */
void afficherTousLesCycles(const Tontine *t);

/* Participants : identifiant et nom. */
void afficherParticipants(const Tontine *t, const Cycle *c);

/*
 * Ordre des bénéficiaires, à partir du tableau c->participants :
 *     Séance 1 -> Membre 3 (nom)
 *     Séance 2 -> Membre 1 (nom) ...
 */
void afficherOrdreBeneficiaires(const Tontine *t, const Cycle *c);

/*
 * Bilan complet (cahier §30) : informations générales, participants,
 * ordre, détail de chaque séance (afficherSeance du module seances),
 * résumé financier (totaux du module cotisations, solde de caisse).
 */
void afficherBilanCycle(const Tontine *t, const Cycle *c);


/* ============================================================
   6. MENUS
   ============================================================ */

/*
 * Sous-menu des cycles (cahier §36) :
 *   1. Créer un cycle                 4. Afficher l'ordre des bénéficiaires
 *   2. Afficher les cycles            5. Consulter un cycle
 *   3. Afficher les participants      6. Consulter le bilan d'un cycle
 *   0. Retour
 * "Créer un cycle" : saisie de l'id, de la date, de la fréquence
 * (en jours), de la cotisation, puis des identifiants des
 * participants (tableau) -> creerCycle(). Bloqué s'il n'y a aucun
 * membre (existeAuMoinsUnMembre). Puis sauvegarderTout(t).
 */
void menuCycles(Tontine *t);

/* Menu principal n°7 "Bilans" : choisit un cycle et affiche son bilan. */
void menuBilans(Tontine *t);

#endif
