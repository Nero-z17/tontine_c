#ifndef UTILS_H
#define UTILS_H

/* ============================================================
   UTILS.H - Dates, affichage commun et fonctions aleatoires
   ============================================================ */

#include "structures.h"


/* ============================================================
   1. DATES  (format JJ/MM/AAAA, tableaux de TAILLE_DATE)
   ============================================================ */

/* Écrit la date du jour dans destination. */
void dateDuJour(char *destination);

/*
 * Écrit dans dateResultat la date dateDepart + nbJours jours.
 * (Conseil : struct tm + mktime de <time.h>.)
 * Exemple : ("01/01/2027", 7, res) -> res = "08/01/2027"
 */
void ajouterJoursADate(const char *dateDepart, int nbJours, char *dateResultat);


/* ============================================================
   2. AFFICHAGE COMMUN
   ============================================================ */

/* Affiche un titre encadré, identique dans tout le programme :
 *   ========================================
 *   TITRE
 *   ========================================
 */
void afficherTitre(const char *titre);

/*
 * Libellés (texte) des énumérations, pour l'affichage.
 * Conseil : pas d'accents dans les libellés (console Windows).
 */
const char *libelleEtatCycle(EtatCycle etat);              /* PLANIFIE / EN COURS / TERMINE */
const char *libelleEtatSeance(EtatSeance etat);            /* OUVERTE / CLOTUREE */
const char *libelleEtatCotisation(EtatCotisation etat);    /* NON PAYEE / PARTIELLE / PAYEE */
const char *libelleEtatDette(EtatDette etat);              /* EN COURS / PARTIELLEMENT REGLEE / REGULARISEE */
const char *libelleTypePenalite(TypePenalite type);        /* PARTIELLE (10%) / NON PAIEMENT (15%) */


/* ============================================================
   3. ALEATOIRE
   ============================================================ */

/* À appeler UNE SEULE FOIS au début de main() : srand(time(NULL)). */
void initialiserAleatoire(void);

/* Retourne un entier aléatoire entre min et max (inclus). */
int nombreAleatoire(int min, int max);

#endif
