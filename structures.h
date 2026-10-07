#ifndef STRUCTURES_H
#define STRUCTURES_H

/* ============================================================
   STRUCTURES.H  (version proposée)
   Projet : Application de gestion d'une tontine
   Langage : C

   Ce fichier contient uniquement les constantes et les
   structures de données. Les fonctions sont déclarées dans
   les fichiers .h de chaque module.

   REGLE D'EQUIPE : une fois validé, ce fichier est "gelé".
   Toute modification doit être annoncée aux 5 membres.
   ============================================================ */


/* ============================================================
   0. CONSTANTES
   ============================================================ */

#define TAILLE_NOM          50
#define TAILLE_TELEPHONE    20
#define TAILLE_LIEU         50
#define TAILLE_DATE         11      /* JJ/MM/AAAA + '\0' */

/* Taux de pénalité (en %) par séance de retard */
#define TAUX_PENALITE_PARTIELLE     10
#define TAUX_PENALITE_NON_PAIEMENT  15


/* ============================================================
   1. TYPES ENUMERES
   ============================================================ */

/* Etat général d'un cycle */
typedef enum{
    CYCLE_PLANIFIE,
    CYCLE_EN_COURS,
    CYCLE_TERMINE
} EtatCycle;

/* Etat d'une séance */
typedef enum{
    SEANCE_OUVERTE,
    SEANCE_CLOTUREE
} EtatSeance;

/* Etat de la cotisation d'un membre pour une séance */
typedef enum{
    COTISATION_NON_PAYEE,
    COTISATION_PARTIELLE,
    COTISATION_PAYEE
} EtatCotisation;

/* Etat d'une dette */
typedef enum{
    DETTE_EN_COURS,
    DETTE_PARTIELLEMENT_REGLEE,
    DETTE_REGULARISEE
} EtatDette;

/* Type de pénalité : détermine le taux appliqué à la dette
 *   PENALITE_PARTIELLE     -> TAUX_PENALITE_PARTIELLE     (10 %)
 *   PENALITE_NON_PAIEMENT  -> TAUX_PENALITE_NON_PAIEMENT  (15 %)
 */
typedef enum{
    PENALITE_PARTIELLE,
    PENALITE_NON_PAIEMENT
} TypePenalite;


/* ============================================================
   2. MEMBRES
   ============================================================ */

/*
 * Un membre est une personne inscrite dans l'application.
 * La liste des membres est une liste chaînée CIRCULAIRE :
 *
 *     Membre 1 -> Membre 2 -> Membre 3
 *        ^                         |
 *        |_________________________|
 */
typedef struct Membre{
    int idMembre;
    char nom[TAILLE_NOM];
    char telephone[TAILLE_TELEPHONE];
    char lieu_de_residence[TAILLE_LIEU];
    struct Membre *suivant;
} Membre;

typedef struct{
    Membre *tete;
    int taille;
} ListeMembres;


/* ============================================================
   3. FILE CIRCULAIRE DES BENEFICIAIRES
   ============================================================ */

/*
 * Un noeud de la file contient l'identifiant d'un membre.
 */
typedef struct NoeudFile{
    int idMembre;
    struct NoeudFile *suivant;
} NoeudFile;

/*
 * File circulaire : ordre fixe des bénéficiaires du cycle.
 *
 *      tete
 *       |
 *       v
 *      [3] -> [7] -> [2] -> [5] -> [9]
 *       ^                              |
 *       |______________________________|
 *
 * CONSEIL : on ne retire pas les noeuds. Pour passer au
 * bénéficiaire suivant, on avance simplement la tête :
 *      file.tete = file.tete->suivant;
 * Ainsi l'ordre n'est jamais perdu (utile pour le bilan).
 */
typedef struct{
    NoeudFile *tete;
    NoeudFile *queue;
    int taille;
} FileCirculaire;


/* ============================================================
   4. SEANCES
   ============================================================ */

/*
 * Une séance = un tour de distribution.
 * Les N séances d'un cycle sont stockées dans un TABLEAU
 * (cycle->seances[0] .. cycle->seances[N-1]).
 * La séance numéro k se trouve donc en seances[k - 1].
 *
 * Les champs "résultat" sont remplis lors de la clôture de
 * la séance ("Passer à la séance suivante"). Ils servent
 * pour l'historique et le bilan.
 *
 * Exemple (5 participants, cotisation 10 000) :
 *      montantBrut      = 50 000
 *      retenuePrincipal =  3 000
 *      retenuePenalites =  2 000
 *      montantNet       = 45 000
 */
typedef struct{
    int idCycle;
    int numeroSeance;               /* de 1 à N */
    char date[TAILLE_DATE];         /* JJ/MM/AAAA */
    int idBeneficiaire;
    EtatSeance etat;

    int montantBrut;                /* nombreParticipants x cotisation */
    int retenuePrincipal;           /* dettes du bénéficiaire (principal) */
    int retenuePenalites;           /* pénalités du bénéficiaire -> caisse */
    int montantNet;                 /* montant réellement reçu */
    int soldeCaisseApres;           /* solde de la caisse après la séance */
} Seance;


/* ============================================================
   5. COTISATIONS
   ============================================================ */

/*
 * Situation d'un membre pour UNE séance précise
 * (uniquement la cotisation COURANTE).
 *
 * Exemple : Cycle 1 - Séance 3 - Membre 7
 *      montantAttendu = 10 000
 *      montantPaye    =  7 000
 *      etat           = COTISATION_PARTIELLE
 *
 * Les anciennes dettes ne sont PAS ici : voir Dette.
 * Insertion conseillée : en fin de liste (ordre chronologique).
 */
typedef struct Cotisation{
    int idCycle;
    int numeroSeance;
    int idMembre;
    int montantAttendu;
    int montantPaye;
    EtatCotisation etat;
    struct Cotisation *suivant;
} Cotisation;


/* ============================================================
   6. DETTES (et pénalités)
   ============================================================ */

/*
 * Une dette = un retard provenant d'une séance précise.
 * Un membre a au plus UNE dette par séance d'origine :
 * la clé d'une dette est donc (idCycle, numeroSeanceOrigine,
 * idDebiteur). Pas besoin d'identifiant séparé.
 *
 * Exemple : séance 1, bénéficiaire = membre 3, débiteur = membre 5
 * Le membre 5 devait 10 000 et a payé 7 000 :
 *      montantPayeInitial = 7 000         ce que le membre a payé à la séance d'origine
 *      principalInitial   = 3 000         ce qui manquait au départ (10 000 − 7 000)
 *      principalRestant   = 3 000 (puis diminue au fil des
 *                                  remboursements)     ce qu'il doit encore rembourser
 *
 * Pénalité par séance de retard =
 *      montantCotisation x taux / 100     10% pour la cotisation partielle et 15% pour l'absence de cotissation
 *      (taux déduit de typePenalite)
 *
 * La dette n'est JAMAIS supprimée : une fois réglée, elle passe
 * à DETTE_REGULARISEE et reste consultable (historique).
 * Insertion conseillée : en fin de liste, pour que les dettes
 * les plus anciennes soient traitées en premier.
 */
typedef struct Dette{
    int idCycle;
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
} Dette;


/* ============================================================
   7. PAIEMENTS
   ============================================================ */

/*
 * Un paiement = une opération réelle effectuée par un membre,
 * pendant une séance précise.
 *
 * Répartition obligatoire :
 *      1) principal ancien   2) pénalités   3) cotisation courante
 *
 * Exemple : montantTotal = 10 000
 *      montantDette = 3 000, montantPenalite = 2 000,
 *      montantCotisationCourante = 5 000
 *
 * Le montantPenalite de chaque paiement entre dans la caisse.
 */
typedef struct Paiement{
    int idCycle;
    int numeroSeance;
    int idMembre;
    char date[TAILLE_DATE];
    int montantTotal;
    int montantDette;
    int montantPenalite;
    int montantCotisationCourante;
    struct Paiement *suivant;
} Paiement;


/* ============================================================
   8. CYCLE
   ============================================================ */

/*
 * Le cycle regroupe TOUTES les données d'une tontine :
 * participants, file, séances, cotisations, dettes,
 * paiements et caisse. Rien n'est partagé entre deux cycles.
 *
 * Nombre de séances = nombre de participants.
 *
 * Caisse : seules les pénalités RÉELLEMENT payées y entrent.
 * Elle démarre à 0. Son évolution est lisible grâce à
 * Seance.soldeCaisseApres.
 */
typedef struct Cycle{
    int idCycle;
    char dateDebut[TAILLE_DATE];
    int frequenceJours;             /* ex : 7, 14, 30 */
    int montantCotisation;          /* fixe pendant tout le cycle */
    int nombreParticipants;         /* = nombre de séances */
    int numeroSeanceActuelle;
    EtatCycle etat;

    int *participants;              /* tableau d'idMembre (taille = nombreParticipants),
                                       dans l'ordre de passage après le tirage */
    FileCirculaire fileBeneficiaires;
    Seance *seances;                /* tableau de nombreParticipants séances */

    Cotisation *cotisations;        /* liste chaînée */
    Dette *dettes;                  /* liste chaînée */
    Paiement *paiements;            /* liste chaînée */

    int soldeCaisse;

    struct Cycle *suivant;          /* liste des cycles */
} Cycle;


/* ============================================================
   9. TONTINE (racine de toutes les données)
   ============================================================ */

/*
 * Une seule variable Tontine est créée dans main.c et
 * transmise (par pointeur) aux fonctions des modules.
 */
typedef struct{
    ListeMembres membres;
    Cycle *cycles;                  /* liste chaînée des cycles */
    int nombreCycles;
} Tontine;


/* ============================================================
   FIN DU FICHIER
   ============================================================ */

#endif
