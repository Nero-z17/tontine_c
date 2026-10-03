#ifndef FICHIERS_H
#define FICHIERS_H

/* ============================================================
   FICHIERS.H - Sauvegarde, chargement, historique, démarrage
   Responsable : Personne 5 (écrit aussi main.c)
   Fichiers    : fichiers.h / fichiers.c / main.c

   Ce module appelle (uniquement des PRIMITIVES, pour ne dépendre
   de personne) :
     - membres.h     : initialiserListeMembres(), creerMembre(), insererMembre(),
                       libererListeMembres()
     - cycles.h      : nouveauCycle(), ajouterCycleFin(), construireFile(),
                       positionnerFile(), libererTousLesCycles()
     - seances.h     : allouerSeances()
     - cotisations.h : nouvelleCotisation(), nouvelleDette(), nouveauPaiement(),
                       ajouterCotisationFin(), ajouterDetteFin(), ajouterPaiementFin()
     - utils.h       : dateDuJour(), lireEntier(), afficherTitre()

   Ce module est appelé par :
     - TOUS les modules : sauvegarderTout(), ajouterHistorique()
     - main.c           : tout le reste

   main.c (à écrire par la Personne 5) :
       Tontine t;
       initialiserAleatoire();
       initialiserTontine(&t);
       chargerTout(&t);
       boucle du menu principal (cahier §34) :
         1 menuMembres   2 menuCycles   3 menuSeances   4 menuCotisations
         5 menuCaisse    6 menuHistorique   7 menuBilans   0 quitter
         -> s'il n'y a aucun membre : afficher le message du cahier §5
            et n'autoriser que le menu 1.
       sauvegarderTout(&t);
       libererTontine(&t);
   ============================================================ */

#include "structures.h"


/* ============================================================
   1. NOMS DES FICHIERS ET CONSTANTES
   ============================================================ */

#define FICHIER_MEMBRES      "membres.txt"
#define FICHIER_FILE         "file.txt"
#define FICHIER_HISTORIQUE   "historique.txt"
#define FICHIER_SEANCE       "seance.txt"
#define FICHIER_COTISATION   "cotisation.txt"
#define FICHIER_SOLDECAISSE  "soldecaisse.txt"

#define TAILLE_MESSAGE_HISTORIQUE  200


/* ============================================================
   2. FORMAT DES FICHIERS (séparateur : le point-virgule ';')
   Les nombres sont écrits en clair, une ligne = un enregistrement.
   Les énumérations sont écrites avec leur valeur entière (cast int).

   membres.txt
       idMembre;nom;telephone;lieu_de_residence

   file.txt   (2 lignes par cycle)
       CYCLE;idCycle;dateDebut;frequenceJours;montantCotisation;
             nombreParticipants;numeroSeanceActuelle;etat;soldeCaisse
       ORDRE;idCycle;id1;id2;...;idN
             (identifiants dans l'ordre de passage = c->participants)

   seance.txt
       idCycle;numeroSeance;date;idBeneficiaire;etat;montantBrut;
       retenuePrincipal;retenuePenalites;montantNet;soldeCaisseApres

   cotisation.txt   (3 types de lignes, repérés par le premier mot)
       COTISATION;idCycle;numeroSeance;idMembre;montantAttendu;
                  montantPaye;etat
       DETTE;idCycle;numeroSeanceOrigine;idDebiteur;idBeneficiaireOrigine;
             montantPayeInitial;principalInitial;principalRestant;
             nombreSeancesRetard;typePenalite;penaliteTotale;penalitePayee;
             etat;numeroSeanceRegularisation
       PAIEMENT;idCycle;numeroSeance;idMembre;date;montantTotal;
                montantDette;montantPenalite;montantCotisationCourante

   soldecaisse.txt   (fichier de CONSULTATION : écrit à chaque
                      sauvegarde, jamais relu)
       idCycle;numeroSeance;soldeApres

   historique.txt   (journal : on AJOUTE des lignes, on n'efface jamais)
       JJ/MM/AAAA;idCycle;numeroSeance;message
       (idCycle = 0 pour un événement qui n'est lié à aucun cycle)
   ============================================================ */


/* ============================================================
   3. DEMARRAGE ET ARRET
   ============================================================ */

/*
 * Met la tontine à zéro : membres vides (initialiserListeMembres),
 * t->cycles = NULL, t->nombreCycles = 0.
 */
void initialiserTontine(Tontine *t);

/* Libère TOUTE la mémoire (membres + cycles). À appeler avant de quitter. */
void libererTontine(Tontine *t);


/* ============================================================
   4. SAUVEGARDE ET CHARGEMENT
   Simplicité : chaque sauvegarde RÉÉCRIT les fichiers en entier
   (les données sont petites). Retour : 1 = succès, 0 = erreur.
   Un fichier absent au premier lancement n'est PAS une erreur :
   la fonction retourne 1 et laisse les données vides.
   ============================================================ */

/* membres.txt */
int sauvegarderMembres(const Tontine *t);
int chargerMembres(Tontine *t);

/*
 * file.txt + seance.txt + cotisation.txt + soldecaisse.txt
 * Chargement, dans cet ordre :
 *   1. file.txt      : crée les cycles (nouveauCycle), remplit
 *                      c->participants, ajouterCycleFin
 *   2. allouerSeances(c) puis seance.txt   : remplit c->seances
 *   3. cotisation.txt : cotisations, dettes, paiements du bon cycle
 *   4. pour chaque cycle : construireFile(c) puis
 *      positionnerFile(c, c->numeroSeanceActuelle)
 */
int sauvegarderCycles(const Tontine *t);
int chargerCycles(Tontine *t);

/* sauvegarderMembres + sauvegarderCycles. */
int sauvegarderTout(const Tontine *t);

/* chargerMembres puis chargerCycles. */
int chargerTout(Tontine *t);


/* ============================================================
   5. HISTORIQUE (cahier §31)
   ============================================================ */

/*
 * Ajoute UNE ligne à la fin de historique.txt (mode "a") :
 *     <date du jour>;idCycle;numeroSeance;message
 * Le message ne doit pas contenir de ';'.
 *
 * QUI ECRIT QUOI (chaque module trace ses propres événements) :
 *   membres      : ajout, modification, suppression d'un membre (idCycle = 0)
 *   cycles       : création du cycle, ordre tiré au sort, fin du cycle
 *   seances      : ouverture et clôture de séance (bénéficiaire, brut,
 *                  retenues, net, solde de caisse)
 *   cotisations  : paiement (répartition), dette créée, pénalité
 *                  appliquée, dette régularisée
 */
void ajouterHistorique(int idCycle, int numeroSeance, const char *message);

/*
 * Affiche l'historique. idCycle = 0 : toutes les lignes ;
 * sinon seulement celles du cycle demandé.
 */
void afficherHistorique(int idCycle);

/*
 * Sous-menu n°6 du menu principal :
 *   1. Afficher tout l'historique
 *   2. Afficher l'historique d'un cycle
 *   0. Retour
 */
void menuHistorique(Tontine *t);

#endif
