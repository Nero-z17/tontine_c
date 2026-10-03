#ifndef UTILS_H
#define UTILS_H

/* ============================================================
   UTILS.H - Fonctions utilitaires communes à tous les modules
   Responsable : Personne 1 (en plus du module membres)
   Utilisé par : TOUS les modules (surtout les menus)

   REGLE D'EQUIPE :
     - Seules les fonctions de ce fichier et les fonctions
       menu*() lisent le clavier (scanf / fgets).
     - Les fonctions de logique ne font JAMAIS de scanf.
   ============================================================ */

#include "structures.h"


/* ============================================================
   1. SAISIE SECURISEE
   ============================================================ */

/* Vide le buffer du clavier (à appeler après un scanf). */
void viderBuffer(void);

/* Affiche "Appuyez sur Entrée pour continuer..." et attend. */
void attendreEntree(void);

/*
 * Affiche le message puis lit un entier compris entre min et max
 * (inclus). Recommence tant que la saisie est invalide.
 * Retourne l'entier valide.
 */
int lireEntier(const char *message, int min, int max);

/*
 * Affiche le message puis lit une ligne (les espaces sont permis).
 * Au plus (taille - 1) caractères sont copiés dans destination.
 * Le '\n' final est supprimé. Les ';' sont remplacés par ',' car
 * le point-virgule sert de séparateur dans les fichiers .txt.
 */
void lireChaine(const char *message, char *destination, int taille);

/*
 * Lit une date au format JJ/MM/AAAA. Recommence tant que
 * dateEstValide() retourne 0. destination : tableau de TAILLE_DATE.
 */
void lireDate(const char *message, char *destination);

/* Pose une question oui/non. Retourne 1 pour oui, 0 pour non. */
int confirmer(const char *message);


/* ============================================================
   2. DATES  (format JJ/MM/AAAA, tableaux de TAILLE_DATE)
   ============================================================ */

/* Retourne 1 si la date est au bon format ET existe vraiment. */
int dateEstValide(const char *date);

/* Écrit la date du jour dans destination. */
void dateDuJour(char *destination);

/*
 * Écrit dans dateResultat la date dateDepart + nbJours jours.
 * (Conseil : struct tm + mktime de <time.h>.)
 * Exemple : ("01/01/2027", 7, res) -> res = "08/01/2027"
 */
void ajouterJoursADate(const char *dateDepart, int nbJours, char *dateResultat);


/* ============================================================
   3. AFFICHAGE COMMUN
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
   4. ALEATOIRE
   ============================================================ */

/* À appeler UNE SEULE FOIS au début de main() : srand(time(NULL)). */
void initialiserAleatoire(void);

/* Retourne un entier aléatoire entre min et max (inclus). */
int nombreAleatoire(int min, int max);

#endif
