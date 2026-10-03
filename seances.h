#ifndef SEANCES_H
#define SEANCES_H

/* ============================================================
   SEANCES.H - Séances, passage à la séance suivante, caisse
   Responsable : Personne 3
   Fichiers    : seances.h / seances.c

   Ce module appelle :
     - cycles.h      : avancerFile(), terminerCycle(), choisirCycle(),
                       cycleAccepteOperations()
     - cotisations.h : creerCotisationsSeance(), creerDettesDepuisSeance(),
                       appliquerPenalites(), regulariserDettesBeneficiaire(),
                       menuCotiser()
     - membres.h     : nomDuMembre()
     - fichiers.h    : sauvegarderTout(), ajouterHistorique()
     - utils.h       : saisies, dates, affichage

   Ce module est appelé par :
     - cycles.c      : initialiserSeances(), ouvrirSeance(), afficherSeance()
     - fichiers.c    : allouerSeances()
     - main.c        : menuSeances(), menuCaisse()

   RAPPEL : les N séances d'un cycle sont dans un TABLEAU :
   la séance numéro k est c->seances[k - 1].
   Une séance dont le numéro est > c->numeroSeanceActuelle est
   "à venir" : elle est OUVERTE dans la structure mais aucune
   opération n'y est permise. Opérations autorisées uniquement
   sur la séance actuelle, tant qu'elle est OUVERTE.
   ============================================================ */

#include "structures.h"


/* Résultat de "Passer à la séance suivante" */
typedef enum{
    CLOTURE_OK,
    CLOTURE_CYCLE_NON_ACTIF,
    CLOTURE_SEANCE_DEJA_CLOTUREE,
    CLOTURE_ERREUR
} ResultatCloture;


/* ============================================================
   1. PRIMITIVES
   ============================================================ */

/*
 * [PRIMITIVE] Alloue (calloc) le tableau de c->nombreParticipants
 * séances, tout à 0. Retourne 1 si OK, 0 sinon.
 * Utilisée par initialiserSeances() et par fichiers.c.
 */
int allouerSeances(Cycle *c);

/*
 * [PRIMITIVE] Retourne l'adresse de la séance numéro numeroSeance
 * (c->seances[numeroSeance - 1]), ou NULL si le numéro n'est pas
 * compris entre 1 et c->nombreParticipants.
 */
Seance *trouverSeance(const Cycle *c, int numeroSeance);

/* [PRIMITIVE] Retourne la séance actuelle du cycle, ou NULL. */
Seance *seanceActuelle(const Cycle *c);


/* ============================================================
   2. CREATION ET OUVERTURE
   ============================================================ */

/*
 * Appelée par creerCycle(), APRES le tirage et la file.
 * Appelle allouerSeances(c) puis, pour k de 1 à N, remplit :
 *     idCycle, numeroSeance = k,
 *     date = dateDebut + (k - 1) x frequenceJours   (ajouterJoursADate),
 *     idBeneficiaire = c->participants[k - 1],
 *     etat = SEANCE_OUVERTE, tous les montants à 0.
 * Retourne 1 si OK, 0 sinon.
 */
int initialiserSeances(Cycle *c);

/*
 * Ouvre la séance numeroSeance : c->numeroSeanceActuelle = numeroSeance
 * puis creerCotisationsSeance(c, numeroSeance) (module cotisations).
 * Retourne 1 si OK, 0 sinon.
 */
int ouvrirSeance(Cycle *c, int numeroSeance);


/* ============================================================
   3. PASSER A LA SEANCE SUIVANTE (cahier §16, §24, §25)
   ============================================================ */

/*
 * Clôture OFFICIELLEMENT la séance actuelle (k) et prépare la
 * suivante. Étapes dans CET ordre (N = nombre de séances) :
 *
 *   0. Vérifier : cycle EN_COURS et séance k pas déjà CLOTUREE
 *      (sinon retourner le code d'erreur : jamais deux fois !).
 *   1. Bénéficiaire :
 *        montantBrut = nombreParticipants x montantCotisation
 *        regulariserDettesBeneficiaire(c, idBeneficiaire, k,
 *                          &retenuePrincipal, &retenuePenalites)
 *        c->soldeCaisse += retenuePenalites
 *        montantNet = montantBrut - retenuePrincipal - retenuePenalites
 *   2. creerDettesDepuisSeance(c, k)   (paiements incomplets ou absents)
 *   3. Si k < N : appliquerPenalites(c, k)
 *        (le retard commence à la séance SUIVANTE)
 *      Si k == N : aucune pénalité (il n'y a plus de séance suivante).
 *   4. séance k : soldeCaisseApres = c->soldeCaisse,
 *                 etat = SEANCE_CLOTUREE (+ tous les champs résultat)
 *   5. Si k < N : avancerFile(c), puis ouvrirSeance(c, k + 1)
 *      Si k == N : terminerCycle(c)
 *   6. ajouterHistorique(...) pour tracer la clôture.
 */
ResultatCloture passerSeanceSuivante(Cycle *c);

/* Texte français expliquant le résultat (à afficher). */
const char *messageCloture(ResultatCloture resultat);


/* ============================================================
   4. AFFICHAGE DES SEANCES
   ============================================================ */

/*
 * Affiche une séance (cahier §9 / §30) : numéro, date, bénéficiaire,
 * état, cotisations de la séance (afficherCotisationsSeance),
 * brut, retenues, net, caisse après la séance.
 */
void afficherSeance(const Tontine *t, const Cycle *c, const Seance *s);

/* Affiche la séance actuelle du cycle. */
void afficherSeanceActuelle(const Tontine *t, const Cycle *c);

/* Tableau récapitulatif de toutes les séances du cycle. */
void afficherHistoriqueSeances(const Tontine *t, const Cycle *c);


/* ============================================================
   5. CAISSE (cahier §26 à §28 et §39)
   Seules les pénalités RÉELLEMENT payées y entrent. Aucune dépense.
   ============================================================ */

/* Solde actuel de la caisse du cycle (c->soldeCaisse). */
void afficherCaisse(const Cycle *c);

/* Tableau : numéro de séance -> solde après la séance (séances clôturées). */
void afficherEvolutionCaisse(const Cycle *c);

/* Solde après la séance numeroSeance (message si pas encore clôturée). */
void afficherSoldeApresSeance(const Cycle *c, int numeroSeance);

/* Solde final (cycle terminé) ou solde actuel avec avertissement. */
void afficherSoldeFinal(const Cycle *c);


/* ============================================================
   6. MENUS
   ============================================================ */

/*
 * Sous-menu des séances (cahier §37) :
 *   1. Afficher la séance actuelle
 *   2. Afficher une séance
 *   3. Enregistrer les opérations de la séance  -> menuCotiser(t, c)
 *   4. Passer à la séance suivante              -> passerSeanceSuivante(c)
 *   5. Afficher l'historique des séances
 *   0. Retour
 * Commence par choisirCycle(t, ...). Après le passage à la séance
 * suivante : sauvegarderTout(t).
 */
void menuSeances(Tontine *t);

/*
 * Sous-menu de la caisse (cahier §39) :
 *   1. Afficher la caisse du cycle     3. Afficher le solde après une séance
 *   2. Afficher l'évolution de la caisse   4. Afficher le solde final
 *   0. Retour
 * AUCUNE fonction de dépense.
 */
void menuCaisse(Tontine *t);

#endif
