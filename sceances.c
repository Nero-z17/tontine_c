#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structures.h"
#include "seances.h"
#include "cycles.h"
#include "cotisations.h"
#include "membres.h"
#include "fichiers.h"
#include "utils.h"


/* ============================================================
   1. PRIMITIVES
   ============================================================ */

/*
 * Alloue le tableau des séances du cycle.
 *
 * Les N séances sont stockées dans :
 *     c->seances[0] ... c->seances[N-1]
 *
 * La séance numéro k est donc :
 *     c->seances[k - 1]
 */
int allouerSeances(Cycle *c)
{
    if (c == NULL || c->nombreParticipants <= 0)
        return 0;

    if (c->seances != NULL)
        return 1;

    c->seances = calloc(
        c->nombreParticipants,
        sizeof(Seance)
    );

    if (c->seances == NULL)
        return 0;

    return 1;
}


/*
 * Recherche une séance par son numéro.
 */
Seance *trouverSeance(const Cycle *c, int numeroSeance)
{
    if (c == NULL || c->seances == NULL)
        return NULL;

    if (numeroSeance < 1 ||
        numeroSeance > c->nombreParticipants)
        return NULL;

    return &c->seances[numeroSeance - 1];
}


/*
 * Retourne la séance actuelle du cycle.
 */
Seance *seanceActuelle(const Cycle *c)
{
    if (c == NULL)
        return NULL;

    return trouverSeance(
        c,
        c->numeroSeanceActuelle
    );
}


/* ============================================================
   2. CREATION ET OUVERTURE
   ============================================================ */

/*
 * Initialise toutes les séances d'un cycle.
 *
 * Les séances futures sont préparées dans le tableau.
 * Seule la séance actuelle accepte réellement les opérations.
 */
int initialiserSeances(Cycle *c)
{
    int k;

    if (c == NULL ||
        c->nombreParticipants <= 0)
        return 0;

    if (!allouerSeances(c))
        return 0;

    for (k = 1; k <= c->nombreParticipants; k++)
    {
        Seance *s;

        s = trouverSeance(c, k);

        if (s == NULL)
            return 0;

        s->idCycle = c->idCycle;
        s->numeroSeance = k;

        /*
         * Date :
         * séance 1 = dateDebut
         * séance 2 = dateDebut + frequenceJours
         * séance 3 = dateDebut + 2 * frequenceJours
         * etc.
         */
        ajouterJoursADate(
            c->dateDebut,
            (k - 1) * c->frequenceJours,
            s->date
        );

        /*
         * Les participants[] sont déjà dans l'ordre
         * des bénéficiaires après le tirage.
         */
        s->idBeneficiaire =
            c->participants[k - 1];

        s->etat = SEANCE_OUVERTE;

        /*
         * Résultats financiers :
         * ils seront remplis lors de la clôture.
         */
        s->montantBrut = 0;
        s->retenuePrincipal = 0;
        s->retenuePenalites = 0;
        s->montantNet = 0;
        s->soldeCaisseApres = 0;
    }

    return 1;
}


/*
 * Ouvre une séance et crée ses cotisations.
 */
int ouvrirSeance(Cycle *c, int numeroSeance)
{
    Seance *s;

    if (c == NULL)
        return 0;

    if (c->etat != CYCLE_EN_COURS)
        return 0;

    s = trouverSeance(c, numeroSeance);

    if (s == NULL)
        return 0;

    /*
     * La séance devient la séance actuelle.
     */
    c->numeroSeanceActuelle = numeroSeance;

    s->etat = SEANCE_OUVERTE;

    /*
     * Création des cotisations des participants
     * pour cette séance.
     */
    if (!creerCotisationsSeance(c, numeroSeance))
        return 0;

    return 1;
}


/* ============================================================
   3. PASSER A LA SEANCE SUIVANTE
   ============================================================ */

ResultatCloture passerSeanceSuivante(Cycle *c)
{
    Seance *s;
    int k;

    int montantBrut;
    int retenuePrincipal;
    int retenuePenalites;
    int montantNet;

    char message[TAILLE_MESSAGE_HISTORIQUE];


    /* --------------------------------------------------------
       Vérification du cycle
       -------------------------------------------------------- */

    if (c == NULL)
        return CLOTURE_ERREUR;

    if (c->etat != CYCLE_EN_COURS)
        return CLOTURE_CYCLE_NON_ACTIF;


    /* --------------------------------------------------------
       Récupération de la séance actuelle
       -------------------------------------------------------- */

    k = c->numeroSeanceActuelle;

    s = seanceActuelle(c);

    if (s == NULL)
        return CLOTURE_ERREUR;

    if (s->etat == SEANCE_CLOTUREE)
        return CLOTURE_SEANCE_DEJA_CLOTUREE;


    /* --------------------------------------------------------
       1. CALCUL DU MONTANT BRUT
       -------------------------------------------------------- */

    montantBrut =
        c->nombreParticipants *
        c->montantCotisation;


    /* --------------------------------------------------------
       2. REGULARISATION DES DETTES DU BENEFICIAIRE
       -------------------------------------------------------- */

    retenuePrincipal = 0;
    retenuePenalites = 0;

    regulariserDettesBeneficiaire(
        c,
        s->idBeneficiaire,
        k,
        &retenuePrincipal,
        &retenuePenalites
    );


    /*
     * Seules les pénalités réellement payées sont ajoutées
     * à la caisse.
     */
    c->soldeCaisse += retenuePenalites;


    /* --------------------------------------------------------
       3. CALCUL DU MONTANT NET
       -------------------------------------------------------- */

    montantNet =
        montantBrut
        - retenuePrincipal
        - retenuePenalites;


    /* --------------------------------------------------------
       4. CREATION DES DETTES DE LA SEANCE
       -------------------------------------------------------- */

    creerDettesDepuisSeance(
        c,
        k
    );


    /* --------------------------------------------------------
       5. APPLICATION DES PENALITES
       -------------------------------------------------------- */

    /*
     * Pour la dernière séance, aucune nouvelle pénalité
     * n'est nécessaire.
     */
    if (k < c->nombreParticipants)
    {
        appliquerPenalites(
            c,
            k
        );
    }


    /* --------------------------------------------------------
       6. ENREGISTREMENT DU RESULTAT DE LA SEANCE
       -------------------------------------------------------- */

    s->montantBrut = montantBrut;

    s->retenuePrincipal =
        retenuePrincipal;

    s->retenuePenalites =
        retenuePenalites;

    s->montantNet =
        montantNet;

    s->soldeCaisseApres =
        c->soldeCaisse;

    s->etat =
        SEANCE_CLOTUREE;


    /* --------------------------------------------------------
       7. PASSAGE A LA SEANCE SUIVANTE
       -------------------------------------------------------- */

    if (k < c->nombreParticipants)
    {
        /*
         * On avance la tête de la file circulaire.
         */
        avancerFile(c);

        /*
         * La séance suivante devient active.
         */
        if (!ouvrirSeance(c, k + 1))
            return CLOTURE_ERREUR;
    }
    else
    {
        /*
         * Toutes les séances sont terminées.
         */
        if (!terminerCycle(c))
            return CLOTURE_ERREUR;
    }


    /* --------------------------------------------------------
       8. HISTORIQUE
       -------------------------------------------------------- */

    snprintf(
        message,
        sizeof(message),
        "Seance %d cloturee. "
        "Beneficiaire ID %d. "
        "Brut = %d FCFA. "
        "Principal retenu = %d FCFA. "
        "Penalites payees = %d FCFA. "
        "Net = %d FCFA. "
        "Caisse = %d FCFA.",
        k,
        s->idBeneficiaire,
        s->montantBrut,
        s->retenuePrincipal,
        s->retenuePenalites,
        s->montantNet,
        s->soldeCaisseApres
    );

    ajouterHistorique(
        c->idCycle,
        k,
        message
    );

    return CLOTURE_OK;
}


/*
 * Retourne le message correspondant au résultat
 * de la clôture.
 */
const char *messageCloture(ResultatCloture resultat)
{
    switch (resultat)
    {
        case CLOTURE_OK:
            return "Seance cloturee avec succes.";

        case CLOTURE_CYCLE_NON_ACTIF:
            return "Le cycle n'est pas actif.";

        case CLOTURE_SEANCE_DEJA_CLOTUREE:
            return "La seance est deja cloturee.";

        case CLOTURE_ERREUR:
        default:
            return "Erreur lors de la cloture de la seance.";
    }
}


/* ============================================================
   4. AFFICHAGE DES SEANCES
   ============================================================ */

void afficherSeance(
    const Tontine *t,
    const Cycle *c,
    const Seance *s)
{
    if (c == NULL || s == NULL)
        return;

    afficherTitre("DETAIL DE LA SEANCE");

    printf("Cycle              : %d\n",
           s->idCycle);

    printf("Numero de seance   : %d / %d\n",
           s->numeroSeance,
           c->nombreParticipants);

    printf("Date               : %s\n",
           s->date);

    printf("Beneficiaire       : ");

    if (t != NULL)
    {
        printf(
            "%s (ID %d)\n",
            nomDuMembre(
                &t->membres,
                s->idBeneficiaire
            ),
            s->idBeneficiaire
        );
    }
    else
    {
        printf(
            "ID %d\n",
            s->idBeneficiaire
        );
    }

    printf(
        "Etat               : %s\n",
        libelleEtatSeance(s->etat)
    );

    printf("\n");

    /*
     * Les résultats financiers sont significatifs
     * après clôture.
     */
    if (s->etat == SEANCE_CLOTUREE)
    {
        printf("MONTANTS\n");
        printf("----------------------------------------\n");

        printf(
            "Montant brut       : %d FCFA\n",
            s->montantBrut
        );

        printf(
            "Retenue principal  : %d FCFA\n",
            s->retenuePrincipal
        );

        printf(
            "Penalites payees   : %d FCFA\n",
            s->retenuePenalites
        );

        printf(
            "Montant net        : %d FCFA\n",
            s->montantNet
        );

        printf(
            "Solde caisse apres : %d FCFA\n",
            s->soldeCaisseApres
        );
    }
    else
    {
        printf(
            "Seance actuellement ouverte.\n"
        );
    }

    printf("\n");

    /*
     * Consultation des cotisations de la séance.
     */
    afficherCotisationsSeance(
        t,
        c,
        s->numeroSeance
    );
}


void afficherSeanceActuelle(
    const Tontine *t,
    const Cycle *c)
{
    Seance *s;

    if (c == NULL)
        return;

    s = seanceActuelle(c);

    if (s == NULL)
    {
        printf("Aucune seance actuelle.\n");
        return;
    }

    afficherSeance(
        t,
        c,
        s
    );
}


void afficherHistoriqueSeances(
    const Tontine *t,
    const Cycle *c)
{
    int k;

    if (c == NULL)
        return;

    afficherTitre(
        "HISTORIQUE DES SEANCES"
    );

    for (k = 1;
         k <= c->nombreParticipants;
         k++)
    {
        Seance *s =
            trouverSeance(c, k);

        if (s == NULL)
            continue;

        printf(
            "\nSeance %d\n",
            s->numeroSeance
        );

        printf(
            "Date          : %s\n",
            s->date
        );

        printf(
            "Beneficiaire  : "
        );

        if (t != NULL)
        {
            printf(
                "%s (ID %d)\n",
                nomDuMembre(
                    &t->membres,
                    s->idBeneficiaire
                ),
                s->idBeneficiaire
            );
        }
        else
        {
            printf(
                "ID %d\n",
                s->idBeneficiaire
            );
        }

        printf(
            "Etat          : %s\n",
            libelleEtatSeance(
                s->etat
            )
        );

        if (s->etat == SEANCE_CLOTUREE)
        {
            printf(
                "Brut          : %d FCFA\n",
                s->montantBrut
            );

            printf(
                "Principal     : %d FCFA\n",
                s->retenuePrincipal
            );

            printf(
                "Penalites     : %d FCFA\n",
                s->retenuePenalites
            );

            printf(
                "Net           : %d FCFA\n",
                s->montantNet
            );

            printf(
                "Caisse        : %d FCFA\n",
                s->soldeCaisseApres
            );
        }
        else
        {
            printf(
                "Statut        : A VENIR / NON CLOTUREE\n"
            );
        }

        printf(
            "----------------------------------------\n"
        );
    }
}


/* ============================================================
   5. CAISSE
   ============================================================ */

void afficherCaisse(const Cycle *c)
{
    if (c == NULL)
        return;

    afficherTitre("CAISSE");

    printf(
        "Cycle        : %d\n",
        c->idCycle
    );

    printf(
        "Solde actuel : %d FCFA\n",
        c->soldeCaisse
    );

    /*
     * Vérification prévue par cotisations.h :
     *
     * c->soldeCaisse ==
     * totalPenalitesPayees(c)
     */
    if (caisseEstCoherente(c))
    {
        printf(
            "Coherence    : OK\n"
        );
    }
    else
    {
        printf(
            "Coherence    : ERREUR - verifier la caisse\n"
        );
    }
}


void afficherEvolutionCaisse(const Cycle *c)
{
    int k;

    if (c == NULL)
        return;

    afficherTitre(
        "EVOLUTION DE LA CAISSE"
    );

    printf(
        "%-10s %-18s %-18s\n",
        "Seance",
        "Penalites payees",
        "Solde caisse"
    );

    printf(
        "------------------------------------------------\n"
    );

    for (k = 1;
         k <= c->nombreParticipants;
         k++)
    {
        Seance *s =
            trouverSeance(c, k);

        if (s == NULL)
            continue;

        if (s->etat == SEANCE_CLOTUREE)
        {
            printf(
                "%-10d %-18d %-18d\n",
                k,
                s->retenuePenalites,
                s->soldeCaisseApres
            );
        }
        else
        {
            printf(
                "%-10d %-18s %-18s\n",
                k,
                "A venir",
                "A venir"
            );
        }
    }
}


void afficherSoldeApresSeance(
    const Cycle *c,
    int numeroSeance)
{
    Seance *s;

    if (c == NULL)
        return;

    s = trouverSeance(
        c,
        numeroSeance
    );

    if (s == NULL)
    {
        printf(
            "Numero de seance invalide.\n"
        );
        return;
    }

    if (s->etat != SEANCE_CLOTUREE)
    {
        printf(
            "La seance %d n'est pas encore cloturee.\n",
            numeroSeance
        );
        return;
    }

    printf(
        "Solde apres la seance %d : %d FCFA\n",
        numeroSeance,
        s->soldeCaisseApres
    );
}


void afficherSoldeFinal(const Cycle *c)
{
    Seance *s;

    if (c == NULL)
        return;

    afficherTitre(
        "SOLDE FINAL"
    );

    if (c->etat != CYCLE_TERMINE)
    {
        printf(
            "Le cycle n'est pas encore termine.\n"
        );

        printf(
            "Solde actuel : %d FCFA\n",
            c->soldeCaisse
        );

        return;
    }

    s = trouverSeance(
        c,
        c->nombreParticipants
    );

    printf(
        "Cycle       : %d\n",
        c->idCycle
    );

    printf(
        "Solde final : %d FCFA\n",
        c->soldeCaisse
    );

    if (s != NULL)
    {
        printf(
            "Derniere seance : %d\n",
            s->numeroSeance
        );
    }
}


/* ============================================================
   6. MENU DES SEANCES
   ============================================================ */

void menuSeances(Tontine *t)
{
    Cycle *c;
    int choix;
    ResultatCloture resultat;

    if (t == NULL)
        return;

    /*
     * Le menu des séances travaille uniquement avec
     * un cycle EN_COURS.
     */
    c = choisirCycle(
        t,
        1
    );

    if (c == NULL)
        return;

    do
    {
        afficherTitre(
            "GESTION DES SEANCES"
        );

        printf(
            "Cycle : %d\n",
            c->idCycle
        );

        printf(
            "Seance actuelle : %d / %d\n",
            c->numeroSeanceActuelle,
            c->nombreParticipants
        );

        printf("\n");

        printf(
            "1. Afficher la seance actuelle\n"
        );

        printf(
            "2. Afficher l'historique des seances\n"
        );

        printf(
            "3. Enregistrer une cotisation\n"
        );

        printf(
            "4. Passer a la seance suivante\n"
        );

        printf(
            "0. Retour\n"
        );

        choix = lireEntier(
            "Votre choix : ",
            0,
            4
        );

        switch (choix)
        {
            case 1:

                afficherSeanceActuelle(
                    t,
                    c
                );

                attendreEntree();

                break;


            case 2:

                afficherHistoriqueSeances(
                    t,
                    c
                );

                attendreEntree();

                break;


            case 3:

                /*
                 * Le dialogue de paiement appartient
                 * au module cotisations.c.
                 */
                menuCotiser(
                    t,
                    c
                );

                break;


            case 4:

                /*
                 * La saisie de confirmation est faite
                 * dans le menu, jamais dans la logique.
                 */
                if (confirmer(
                    "Voulez-vous cloturer la seance actuelle ?"))
                {
                    resultat =
                        passerSeanceSuivante(c);

                    printf(
                        "\n%s\n",
                        messageCloture(
                            resultat
                        )
                    );

                    if (resultat == CLOTURE_OK)
                    {
                        /*
                         * Sauvegarde centralisée du projet.
                         */
                        sauvegarderTout(t);

                        if (c->etat == CYCLE_EN_COURS)
                        {
                            printf(
                                "La seance %d est maintenant ouverte.\n",
                                c->numeroSeanceActuelle
                            );

                            printf(
                                "Beneficiaire : %s\n",
                                nomDuMembre(
                                    &t->membres,
                                    c->seances[
                                        c->numeroSeanceActuelle - 1
                                    ].idBeneficiaire
                                )
                            );
                        }
                        else
                        {
                            printf(
                                "Le cycle %d est termine.\n",
                                c->idCycle
                            );
                        }
                    }

                    attendreEntree();
                }

                break;
        }

    } while (choix != 0);
}


/* ============================================================
   7. MENU DE LA CAISSE
   ============================================================ */

void menuCaisse(Tontine *t)
{
    Cycle *c;
    int choix;
    int numeroSeance;

    if (t == NULL)
        return;

    /*
     * 0 signifie qu'on peut consulter un cycle même
     * s'il est terminé.
     */
    c = choisirCycle(
        t,
        0
    );

    if (c == NULL)
        return;

    do
    {
        afficherTitre(
            "GESTION DE LA CAISSE"
        );

        printf(
            "Cycle : %d\n",
            c->idCycle
        );

        printf("\n");

        printf(
            "1. Afficher la caisse actuelle\n"
        );

        printf(
            "2. Afficher l'evolution de la caisse\n"
        );

        printf(
            "3. Afficher le solde apres une seance\n"
        );

        printf(
            "4. Afficher le solde final\n"
        );

        printf(
            "0. Retour\n"
        );

        choix = lireEntier(
            "Votre choix : ",
            0,
            4
        );

        switch (choix)
        {
            case 1:

                afficherCaisse(c);

                attendreEntree();

                break;


            case 2:

                afficherEvolutionCaisse(c);

                attendreEntree();

                break;


            case 3:

                numeroSeance = lireEntier(
                    "Numero de la seance : ",
                    1,
                    c->nombreParticipants
                );

                afficherSoldeApresSeance(
                    c,
                    numeroSeance
                );

                attendreEntree();

                break;


            case 4:

                afficherSoldeFinal(c);

                attendreEntree();

                break;
        }

    } while (choix != 0);
}