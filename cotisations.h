#ifndef COTISATIONS_H
#define COTISATIONS_H

/* ============================================================
   COTISATIONS.H - Cotisations, paiements, dettes, pénalités
   Responsable : Personne 4  (le module le plus délicat : à
   confier à la personne la plus à l'aise)
   Fichiers    : cotisations.h / cotisations.c

   Ce module appelle :
     - cycles.h   : membreParticipeAuCycle(), cycleAccepteOperations(),
                    choisirCycle()
     - seances.h  : trouverSeance(), seanceActuelle()
     - membres.h  : nomDuMembre()
     - fichiers.h : sauvegarderTout(), ajouterHistorique()
     - utils.h    : saisies, dates, affichage, libellés

   Ce module est appelé par :
     - seances.c  : creerCotisationsSeance(), creerDettesDepuisSeance(),
                    appliquerPenalites(), regulariserDettesBeneficiaire(),
                    afficherCotisationsSeance(), menuCotiser()
     - cycles.c   : total...() pour le bilan
     - fichiers.c : nouvelleCotisation(), nouvelleDette(), nouveauPaiement(),
                    ajouter...Fin()
     - main.c     : menuCotisations()

   LES 4 REGLES D'OR (cahier §12, §13, §23, §44) :
     1. Imputation d'un paiement : principal des anciennes dettes,
        puis pénalités, puis cotisation courante.
     2. Un paiement supérieur au total dû est REFUSE.
     3. Seules les pénalités RÉELLEMENT payées entrent dans la caisse
        (c->soldeCaisse += montantPenalite, au moment du paiement).
     4. Une dette n'est jamais supprimée (état DETTE_REGULARISEE).

   VERIFICATION POUR LES TESTS :
     c->soldeCaisse == totalPenalitesPayees(c)  doit toujours être vrai.
   ============================================================ */

#include "structures.h"


/* ============================================================
   0. TYPES DE CE MODULE
   ============================================================ */

/* Résultat d'un paiement */
typedef enum{
    PAIEMENT_OK,
    PAIEMENT_MONTANT_INVALIDE,        /* montant <= 0 */
    PAIEMENT_SUPERIEUR_AU_DU,         /* surpaiement : refusé (cahier §13) */
    PAIEMENT_MEMBRE_NON_PARTICIPANT,
    PAIEMENT_CYCLE_NON_ACTIF,
    PAIEMENT_SEANCE_CLOTUREE,
    PAIEMENT_RIEN_A_PAYER,            /* total dû = 0 */
    PAIEMENT_ERREUR_MEMOIRE
} ResultatPaiement;

/*
 * Détail de ce qu'un membre doit AUJOURD'HUI (séance actuelle).
 *   principalDu           : somme des principalRestant des dettes non régularisées
 *   penalitesDues         : somme des pénalités calculées - payées
 *   cotisationCouranteDue : montantAttendu - montantPaye (séance actuelle)
 *   totalDu               : somme des trois
 */
typedef struct{
    int principalDu;
    int penalitesDues;
    int cotisationCouranteDue;
    int totalDu;
} DetailDu;

/*
 * Répartition d'un paiement (même ordre que l'imputation) :
 * exemple : paie 10 000 avec 3 000 de principal, 2 000 de pénalités
 *           -> montantDette = 3 000, montantPenalite = 2 000,
 *              montantCotisationCourante = 5 000
 */
typedef struct{
    int montantDette;
    int montantPenalite;
    int montantCotisationCourante;
} RepartitionPaiement;


/* ============================================================
   1. PRIMITIVES (à écrire EN PREMIER : fichiers.c en a besoin)
   ============================================================ */

/*
 * [PRIMITIVE] Allouent (calloc) un enregistrement VIDE :
 * tous les champs à 0, suivant = NULL. Retournent NULL si échec.
 * (fichiers.c les utilise puis remplit les champs.)
 */
Cotisation *nouvelleCotisation(void);
Dette *nouvelleDette(void);
Paiement *nouveauPaiement(void);

/*
 * [PRIMITIVE] Ajoutent l'élément EN FIN de la liste du cycle
 * (ordre chronologique : les dettes les plus anciennes d'abord).
 * Retournent 1 si OK, 0 sinon.
 */
int ajouterCotisationFin(Cycle *c, Cotisation *cotisation);
int ajouterDetteFin(Cycle *c, Dette *dette);
int ajouterPaiementFin(Cycle *c, Paiement *paiement);

/* [PRIMITIVE] Libèrent toute la liste (appelées par libererCycle). */
void libererCotisations(Cotisation *tete);
void libererDettes(Dette *tete);
void libererPaiements(Paiement *tete);

/* [PRIMITIVE] Retournent l'élément trouvé, ou NULL. */
Cotisation *trouverCotisation(const Cycle *c, int numeroSeance, int idMembre);
Dette *trouverDette(const Cycle *c, int numeroSeanceOrigine, int idDebiteur);


/* ============================================================
   2. CALCULS (fonctions pures : aucun affichage, aucune saisie)
   ============================================================ */

/*
 * Pénalité pour UNE séance de retard :
 *     c->montantCotisation x taux / 100
 * taux = TAUX_PENALITE_PARTIELLE (10) ou TAUX_PENALITE_NON_PAIEMENT (15)
 * selon le type. Exemple : cotisation 10 000, partielle -> 1 000.
 */
int penaliteParSeance(const Cycle *c, TypePenalite type);

/* penaliteTotale - penalitePayee */
int penaliteRestante(const Dette *dette);

/* principalRestant + penaliteRestante */
int detteRestante(const Dette *dette);

/*
 * Ce que le membre doit aujourd'hui, à la séance actuelle du cycle.
 * Si la séance actuelle est clôturée ou inexistante : cotisation
 * courante due = 0.
 */
DetailDu calculerDetailDu(const Cycle *c, int idMembre);

/*
 * Répartit "montant" selon l'ordre d'imputation, SANS rien modifier.
 * Suppose montant <= du->totalDu (le refus du surpaiement est fait
 * par effectuerPaiement). Pure fonction : facile à tester seule.
 */
RepartitionPaiement calculerRepartition(const DetailDu *du, int montant);


/* ============================================================
   3. OPERATIONS
   ============================================================ */

/*
 * Crée une Cotisation (NON_PAYEE, montantAttendu = c->montantCotisation,
 * montantPaye = 0) pour CHAQUE participant, pour la séance numeroSeance.
 * Ne crée pas de doublon si elles existent déjà.
 * Appelée par ouvrirSeance(). Retourne 1 si OK, 0 sinon.
 */
int creerCotisationsSeance(Cycle *c, int numeroSeance);

/*
 * Enregistre un paiement de "montant" pour le membre, à la séance
 * actuelle. Étapes :
 *   1. Vérifier : cycle EN_COURS, séance ouverte, membre participant,
 *      montant > 0, total dû > 0.
 *   2. d = calculerDetailDu(); si montant > d.totalDu -> REFUSE.
 *   3. r = calculerRepartition(&d, montant).
 *   4. Dettes (de la plus ancienne à la plus récente) :
 *        d'abord le principal (r.montantDette), puis les pénalités
 *        (r.montantPenalite). Mettre à jour principalRestant,
 *        penalitePayee, etat (EN_COURS -> PARTIELLEMENT_REGLEE ->
 *        REGULARISEE) et numeroSeanceRegularisation.
 *   5. Cotisation courante : montantPaye += r.montantCotisationCourante,
 *      etat (NON_PAYEE / PARTIELLE / PAYEE).
 *   6. Créer le Paiement (numeroSeance = séance actuelle) et l'ajouter.
 *   7. c->soldeCaisse += r.montantPenalite.
 *   8. ajouterHistorique(...).
 * "recu" (peut être NULL) : reçoit une copie du paiement enregistré
 * pour l'afficher. "date" : JJ/MM/AAAA.
 */
ResultatPaiement effectuerPaiement(Cycle *c, int idMembre, int montant,
                                   const char *date, Paiement *recu);

/*
 * Menu "Régulariser une dette" : comme effectuerPaiement, mais le
 * paiement est limité aux DETTES (principalDu + penalitesDues) :
 * il ne touche jamais à la cotisation courante. Dépasser ce total
 * -> PAIEMENT_SUPERIEUR_AU_DU.
 */
ResultatPaiement regulariserDette(Cycle *c, int idMembre, int montant,
                                  const char *date, Paiement *recu);

/* Texte français expliquant le résultat (à afficher). */
const char *messagePaiement(ResultatPaiement resultat);


/* ============================================================
   4. OPERATIONS APPELEES PAR LA CLOTURE D'UNE SEANCE
   (utilisées par passerSeanceSuivante, module seances)
   ============================================================ */

/*
 * Pour chaque Cotisation de la séance dont l'état n'est pas
 * COTISATION_PAYEE, crée une Dette :
 *     numeroSeanceOrigine = numeroSeance, idDebiteur = idMembre,
 *     idBeneficiaireOrigine = bénéficiaire de cette séance,
 *     montantPayeInitial = montantPaye,
 *     principalInitial = principalRestant = montantAttendu - montantPaye,
 *     typePenalite = PENALITE_PARTIELLE si montantPaye > 0,
 *                    sinon PENALITE_NON_PAIEMENT,
 *     etat = DETTE_EN_COURS, nombreSeancesRetard = 0, pénalités = 0.
 * Ne recrée pas une dette qui existe déjà (trouverDette).
 * Retourne le nombre de dettes créées.
 */
int creerDettesDepuisSeance(Cycle *c, int numeroSeance);

/*
 * Pour chaque dette NON régularisée avec numeroSeanceOrigine <=
 * numeroSeance : nombreSeancesRetard doit valoir
 *     (numeroSeance - numeroSeanceOrigine + 1).
 * Tant qu'il est plus petit : nombreSeancesRetard++ et
 *     penaliteTotale += penaliteParSeance(c, typePenalite).
 * -> impossible de pénaliser deux fois la même séance (Règle 8).
 * Les pénalités calculées mais non payées n'entrent PAS en caisse.
 */
void appliquerPenalites(Cycle *c, int numeroSeance);

/*
 * Au tour d'un bénéficiaire (cahier §24-§25) : pour chaque dette non
 * régularisée dont il est le débiteur :
 *     *retenuePrincipal += principalRestant ; principalRestant = 0
 *     *retenuePenalites += penaliteRestante ; penalitePayee = penaliteTotale
 *     etat = DETTE_REGULARISEE ; numeroSeanceRegularisation = numeroSeance
 * Ne touche PAS à la caisse (c'est seances.c qui ajoute
 * *retenuePenalites à c->soldeCaisse). Les deux sorties sont
 * mises à 0 au départ de la fonction.
 */
void regulariserDettesBeneficiaire(Cycle *c, int idBeneficiaire,
                                   int numeroSeance,
                                   int *retenuePrincipal,
                                   int *retenuePenalites);


/* ============================================================
   5. TOTAUX (pour le bilan, cahier §30 "Résumé financier")
   ============================================================ */

/* nombreParticipants x nombreParticipants x montantCotisation */
int totalCotisationsAttendues(const Cycle *c);

/* Somme des montantTotal de tous les Paiement (argent réellement versé). */
int totalEncaisse(const Cycle *c);

/* Somme des principalRestant de toutes les dettes (dettes restantes). */
int totalPrincipalRestant(const Cycle *c);

/* Somme des penaliteTotale (pénalités calculées). */
int totalPenalitesCalculees(const Cycle *c);

/* Somme des penalitePayee (pénalités réellement encaissées). */
int totalPenalitesPayees(const Cycle *c);

/* Retourne 1 si c->soldeCaisse == totalPenalitesPayees(c) (test de cohérence). */
int caisseEstCoherente(const Cycle *c);


/* ============================================================
   6. AFFICHAGE
   ============================================================ */

/* Affiche clairement les 4 lignes : principal, pénalités, cotisation, total. */
void afficherDetailDu(const DetailDu *du);

/* Cotisations de tous les participants pour une séance. */
void afficherCotisationsSeance(const Tontine *t, const Cycle *c, int numeroSeance);

/* Cotisations d'un membre, séance par séance. */
void afficherCotisationsMembre(const Tontine *t, const Cycle *c, int idMembre);

/*
 * Toutes les dettes du cycle, y compris régularisées : séance d'origine,
 * débiteur, bénéficiaire, principal initial / restant, retards, état.
 */
void afficherDettes(const Tontine *t, const Cycle *c);

/* Pénalités par dette : calculées, payées, restantes. */
void afficherPenalites(const Tontine *t, const Cycle *c);

/* Historique des paiements du cycle. */
void afficherPaiements(const Tontine *t, const Cycle *c);


/* ============================================================
   7. MENUS
   ============================================================ */

/*
 * Sous-menu des cotisations (cahier §38) :
 *   1. Cotiser                      4. Consulter les pénalités
 *   2. Consulter les cotisations    5. Régulariser une dette
 *   3. Consulter les dettes         0. Retour
 * Commence par choisirCycle(t, ...). Après chaque paiement accepté :
 * sauvegarderTout(t).
 */
void menuCotisations(Tontine *t);

/*
 * Dialogue pour UN paiement dans le cycle c : demande l'identifiant du
 * membre, affiche afficherDetailDu(), demande le montant, appelle
 * effectuerPaiement(), affiche messagePaiement() et le reçu.
 * Utilisée par menuCotisations (option 1) ET par menuSeances (option 3).
 */
void menuCotiser(Tontine *t, Cycle *c);

#endif
