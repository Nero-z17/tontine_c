#ifndef MEMBRES_H
#define MEMBRES_H

/* ============================================================
   MEMBRES.H - Gestion des membres (liste chaînée circulaire)
   Responsable : Personne 1
   Fichiers    : membres.h / membres.c

   Ce module appelle :
     - cycles.h    : membreAUnCycleNonTermine()
     - fichiers.h  : sauvegarderTout(), ajouterHistorique()
     - utils.h     : affichage

   Ce module est appelé par :
     - fichiers.c  : creerMembre(), insererMembre()   (chargement)
     - cycles.c, seances.c, cotisations.c : nomDuMembre(),
       trouverMembreParId(), existeAuMoinsUnMembre()
     - main.c      : menuMembres()

   REGLES :
     - Les fonctions marquées [PRIMITIVE] sont à écrire EN PREMIER
       (les autres modules en ont besoin tout de suite).
     - Les fonctions de logique ne lisent pas le clavier et ne
       sauvegardent pas : c'est le menu qui appelle
       sauvegarderTout(t) après chaque opération réussie.
   ============================================================ */

#include "structures.h"


/* Résultat d'une tentative de suppression (cahier §4.4) */
typedef enum{
    SUPPRESSION_OK,
    SUPPRESSION_MEMBRE_INTROUVABLE,
    SUPPRESSION_REFUSEE_CYCLE_NON_TERMINE
} ResultatSuppression;


/* ============================================================
   1. PRIMITIVES (liste circulaire)
   ============================================================ */

/* [PRIMITIVE] Liste vide : tete = NULL, taille = 0. */
void initialiserListeMembres(ListeMembres *liste);

/*
 * [PRIMITIVE] Alloue un membre (malloc) et remplit ses champs.
 * suivant = NULL. Retourne NULL si l'allocation échoue.
 */
Membre *creerMembre(int idMembre, const char *nom,
                    const char *telephone, const char *lieu_de_residence);

/*
 * [PRIMITIVE] Insère le membre EN FIN de liste circulaire
 * (le dernier membre pointe vers la tête). Incrémente taille.
 * Retourne 0 (sans rien insérer) si l'identifiant existe déjà,
 * 1 sinon.
 */
int insererMembre(ListeMembres *liste, Membre *membre);

/* [PRIMITIVE] Retourne le membre d'identifiant idMembre, ou NULL. */
Membre *trouverMembreParId(const ListeMembres *liste, int idMembre);

/* [PRIMITIVE] Retourne 1 si l'identifiant existe, 0 sinon. */
int idMembreExiste(const ListeMembres *liste, int idMembre);

/* [PRIMITIVE] Retourne 1 s'il y a au moins un membre (cahier §5). */
int existeAuMoinsUnMembre(const ListeMembres *liste);

/*
 * [PRIMITIVE] Retourne le nom du membre pour l'affichage.
 * Si le membre n'existe pas, retourne "Inconnu". Ne retourne
 * JAMAIS NULL (pratique dans un printf).
 */
const char *nomDuMembre(const ListeMembres *liste, int idMembre);

/* [PRIMITIVE] Libère tous les membres (attention : liste circulaire !). */
void libererListeMembres(ListeMembres *liste);


/* ============================================================
   2. OPERATIONS (logique, sans clavier)
   ============================================================ */

/*
 * Crée un membre et l'insère dans t->membres.
 * Vérifie : idMembre > 0, identifiant libre, nom non vide.
 * Retourne 1 si le membre est ajouté, 0 sinon.
 */
int ajouterMembre(Tontine *t, int idMembre, const char *nom,
                  const char *telephone, const char *lieu_de_residence);

/*
 * Modifie les informations PERSONNELLES (nom, téléphone, lieu).
 * L'identifiant ne change jamais (l'historique financier non plus).
 * Retourne 1 si le membre existe, 0 sinon.
 */
int modifierMembre(Tontine *t, int idMembre, const char *nouveauNom,
                   const char *nouveauTelephone, const char *nouveauLieu);

/*
 * Supprime un membre SEULEMENT si tous ses cycles sont terminés :
 * utiliser membreAUnCycleNonTermine(t, idMembre) (module cycles).
 * Attention à la suppression dans une liste circulaire :
 * premier membre, dernier membre, membre unique.
 */
ResultatSuppression supprimerMembre(Tontine *t, int idMembre);

/* Texte français expliquant le résultat (à afficher à l'utilisateur). */
const char *messageSuppression(ResultatSuppression resultat);


/* ============================================================
   3. AFFICHAGE ET RECHERCHE
   ============================================================ */

/* Affiche un membre sur une ligne. */
void afficherMembre(const Membre *membre);

/* Affiche tous les membres (ou un message s'il n'y en a aucun). */
void afficherTousLesMembres(const ListeMembres *liste);

/*
 * Affiche les membres dont le nom CONTIENT le texte (sans tenir
 * compte des majuscules). Retourne le nombre de membres trouvés.
 * (La recherche par identifiant se fait avec trouverMembreParId.)
 */
int afficherMembresParNom(const ListeMembres *liste, const char *texte);


/* ============================================================
   4. MENU (cahier §35)
   ============================================================ */

/*
 * Sous-menu :
 *   1. Ajouter un membre      4. Afficher les membres
 *   2. Modifier un membre     5. Supprimer un membre
 *   3. Rechercher un membre   0. Retour
 * Après chaque opération réussie : sauvegarderTout(t) et
 * ajouterHistorique(0, 0, "...").
 */
void menuMembres(Tontine *t);

#endif
