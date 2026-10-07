#include <stdio.h>
#include <stdlib.h>

#include "seances.h"
#include "cycles.h"
#include "cotisations.h"
#include "membres.h"
#include "fichiers.h"
#include "utils.h"

int allouerSeances(Cycle *c)
{
    if (!c || c->nombreParticipants <= 0)
        return 0;
    if (c->seances)
        return 1;
    c->seances = calloc((size_t)c->nombreParticipants, sizeof(*c->seances));
    return c->seances != NULL;
}

Seance *trouverSeance(const Cycle *c, int numeroSeance)
{
    if (!c || !c->seances || numeroSeance < 1 ||
        numeroSeance > c->nombreParticipants)
        return NULL;
    return &c->seances[numeroSeance - 1];
}

Seance *seanceActuelle(const Cycle *c)
{
    return c ? trouverSeance(c, c->numeroSeanceActuelle) : NULL;
}

int initialiserSeances(Cycle *c)
{
    int k;
    if (!c || c->nombreParticipants <= 0 || !c->participants ||
        !allouerSeances(c))
        return 0;

    for (k = 1; k <= c->nombreParticipants; k++) {
        Seance *s = trouverSeance(c, k);
        if (!s)
            return 0;
        s->idCycle = c->idCycle;
        s->numeroSeance = k;
        ajouterJoursADate(c->dateDebut, (k - 1) * c->frequenceJours, s->date);
        s->idBeneficiaire = c->participants[k - 1];
        s->etat = SEANCE_OUVERTE;
        s->montantBrut = 0;
        s->retenuePrincipal = 0;
        s->retenuePenalites = 0;
        s->montantNet = 0;
        s->soldeCaisseApres = 0;
    }
    return 1;
}

int ouvrirSeance(Cycle *c, int numeroSeance)
{
    Seance *s;
    if (!c || c->etat != CYCLE_EN_COURS ||
        !(s = trouverSeance(c, numeroSeance)))
        return 0;
    c->numeroSeanceActuelle = numeroSeance;
    s->etat = SEANCE_OUVERTE;
    return creerCotisationsSeance(c, numeroSeance);
}

ResultatCloture passerSeanceSuivante(Cycle *c)
{
    Seance *s;
    int k, montantBrut, retenuePrincipal = 0, retenuePenalites = 0;
    char message[TAILLE_MESSAGE_HISTORIQUE];

    if (!c)
        return CLOTURE_ERREUR;
    if (c->etat != CYCLE_EN_COURS)
        return CLOTURE_CYCLE_NON_ACTIF;
    k = c->numeroSeanceActuelle;
    s = seanceActuelle(c);
    if (!s)
        return CLOTURE_ERREUR;
    if (s->etat == SEANCE_CLOTUREE)
        return CLOTURE_SEANCE_DEJA_CLOTUREE;

    montantBrut = c->nombreParticipants * c->montantCotisation;
    regulariserDettesBeneficiaire(c, s->idBeneficiaire, k,
                                  &retenuePrincipal, &retenuePenalites);
    c->soldeCaisse += retenuePenalites;
    s->montantBrut = montantBrut;
    s->retenuePrincipal = retenuePrincipal;
    s->retenuePenalites = retenuePenalites;
    s->montantNet = montantBrut - retenuePrincipal - retenuePenalites;
    s->soldeCaisseApres = c->soldeCaisse;

    creerDettesDepuisSeance(c, k);
    if (k < c->nombreParticipants)
        appliquerPenalites(c, k);
    s->etat = SEANCE_CLOTUREE;

    if (k < c->nombreParticipants) {
        avancerFile(c);
        if (!ouvrirSeance(c, k + 1))
            return CLOTURE_ERREUR;
    } else if (!terminerCycle(c)) {
        return CLOTURE_ERREUR;
    }

    snprintf(message, sizeof(message),
             "Seance %d cloturee. Beneficiaire ID %d. Brut = %d FCFA. "
             "Principal retenu = %d FCFA. Penalites payees = %d FCFA. "
             "Net = %d FCFA. Caisse = %d FCFA.",
             k, s->idBeneficiaire, s->montantBrut, s->retenuePrincipal,
             s->retenuePenalites, s->montantNet, s->soldeCaisseApres);
    ajouterHistorique(c->idCycle, k, message);
    return CLOTURE_OK;
}

const char *messageCloture(ResultatCloture resultat)
{
    switch (resultat) {
        case CLOTURE_OK: return "Seance cloturee avec succes.";
        case CLOTURE_CYCLE_NON_ACTIF: return "Le cycle n'est pas actif.";
        case CLOTURE_SEANCE_DEJA_CLOTUREE: return "La seance est deja cloturee.";
        case CLOTURE_ERREUR: return "Erreur lors de la cloture de la seance.";
    }
    return "Erreur lors de la cloture de la seance.";
}

void afficherSeance(const Tontine *t, const Cycle *c, const Seance *s)
{
    if (!c || !s)
        return;
    afficherTitre("DETAIL DE LA SEANCE");
    printf("Cycle              : %d\n", s->idCycle);
    printf("Numero de seance   : %d / %d\n", s->numeroSeance,
           c->nombreParticipants);
    printf("Date               : %s\n", s->date);
    if (t)
        printf("Beneficiaire       : %s (ID %d)\n",
               nomDuMembre(&t->membres, s->idBeneficiaire),
               s->idBeneficiaire);
    else
        printf("Beneficiaire       : ID %d\n", s->idBeneficiaire);
    printf("Etat               : %s\n\n", libelleEtatSeance(s->etat));
    if (s->etat == SEANCE_CLOTUREE) {
        printf("MONTANTS\n----------------------------------------\n");
        printf("Montant brut       : %d FCFA\n", s->montantBrut);
        printf("Retenue principal  : %d FCFA\n", s->retenuePrincipal);
        printf("Penalites payees   : %d FCFA\n", s->retenuePenalites);
        printf("Montant net        : %d FCFA\n", s->montantNet);
        printf("Solde caisse apres : %d FCFA\n", s->soldeCaisseApres);
    } else {
        puts("Seance actuellement ouverte.");
    }
    puts("");
    afficherCotisationsSeance(t, c, s->numeroSeance);
}

void afficherSeanceActuelle(const Tontine *t, const Cycle *c)
{
    Seance *s;
    if (!c)
        return;
    s = seanceActuelle(c);
    if (!s) {
        puts("Aucune seance actuelle.");
        return;
    }
    afficherSeance(t, c, s);
}

void afficherHistoriqueSeances(const Tontine *t, const Cycle *c)
{
    int k;
    if (!c)
        return;
    afficherTitre("HISTORIQUE DES SEANCES");
    for (k = 1; k <= c->nombreParticipants; k++) {
        Seance *s = trouverSeance(c, k);
        if (!s)
            continue;
        printf("\nSeance %d\nDate          : %s\n",
               s->numeroSeance, s->date);
        if (t)
            printf("Beneficiaire  : %s (ID %d)\n",
                   nomDuMembre(&t->membres, s->idBeneficiaire),
                   s->idBeneficiaire);
        else
            printf("Beneficiaire  : ID %d\n", s->idBeneficiaire);
        printf("Etat          : %s\n", libelleEtatSeance(s->etat));
        if (s->etat == SEANCE_CLOTUREE) {
            printf("Brut          : %d FCFA\n", s->montantBrut);
            printf("Principal     : %d FCFA\n", s->retenuePrincipal);
            printf("Penalites     : %d FCFA\n", s->retenuePenalites);
            printf("Net           : %d FCFA\n", s->montantNet);
            printf("Caisse        : %d FCFA\n", s->soldeCaisseApres);
        } else {
            puts("Statut        : A VENIR / NON CLOTUREE");
        }
        puts("----------------------------------------");
    }
}

void afficherCaisse(const Cycle *c)
{
    if (!c)
        return;
    afficherTitre("CAISSE");
    printf("Cycle        : %d\nSolde actuel : %d FCFA\n",
           c->idCycle, c->soldeCaisse);
    puts(caisseEstCoherente(c) ? "Coherence    : OK" :
         "Coherence    : ERREUR - verifier la caisse");
}

void afficherEvolutionCaisse(const Cycle *c)
{
    int k;
    if (!c)
        return;
    afficherTitre("EVOLUTION DE LA CAISSE");
    printf("%-10s %-18s %-18s\n", "Seance", "Penalites payees",
           "Solde caisse");
    puts("------------------------------------------------");
    for (k = 1; k <= c->nombreParticipants; k++) {
        Seance *s = trouverSeance(c, k);
        if (!s)
            continue;
        if (s->etat == SEANCE_CLOTUREE)
            printf("%-10d %-18d %-18d\n", k, s->retenuePenalites,
                   s->soldeCaisseApres);
        else
            printf("%-10d %-18s %-18s\n", k, "A venir", "A venir");
    }
}

void afficherSoldeApresSeance(const Cycle *c, int numeroSeance)
{
    Seance *s;
    if (!c)
        return;
    s = trouverSeance(c, numeroSeance);
    if (!s) {
        puts("Numero de seance invalide.");
        return;
    }
    if (s->etat != SEANCE_CLOTUREE) {
        printf("La seance %d n'est pas encore cloturee.\n", numeroSeance);
        return;
    }
    printf("Solde apres la seance %d : %d FCFA\n",
           numeroSeance, s->soldeCaisseApres);
}

void afficherSoldeFinal(const Cycle *c)
{
    Seance *s;
    if (!c)
        return;
    afficherTitre("SOLDE FINAL");
    if (c->etat != CYCLE_TERMINE) {
        printf("Le cycle n'est pas encore termine.\nSolde actuel : %d FCFA\n",
               c->soldeCaisse);
        return;
    }
    s = trouverSeance(c, c->nombreParticipants);
    printf("Cycle       : %d\nSolde final : %d FCFA\n",
           c->idCycle, c->soldeCaisse);
    if (s)
        printf("Derniere seance : %d\n", s->numeroSeance);
}

void menuSeances(Tontine *t)
{
    Cycle *c;
    int choix;
    ResultatCloture resultat;
    if (!t || !(c = choisirCycle(t, 1)))
        return;

    do {
        char confirmation[4];
        afficherTitre("GESTION DES SEANCES");
        printf("Cycle : %d\nSeance actuelle : %d / %d\n\n",
               c->idCycle, c->numeroSeanceActuelle, c->nombreParticipants);
        puts("1. Afficher la seance actuelle");
        puts("2. Afficher l'historique des seances");
        puts("3. Enregistrer une cotisation");
        puts("4. Passer a la seance suivante");
        puts("0. Retour");
        printf("Votre choix : ");
        scanf("%d", &choix);
        switch (choix) {
            case 1:
                afficherSeanceActuelle(t, c);
                break;
            case 2:
                afficherHistoriqueSeances(t, c);
                break;
            case 3:
                menuCotiser(t, c);
                break;
            case 4:
                printf("Voulez-vous cloturer la seance actuelle ? (o/n) : ");
                scanf(" %3s", confirmation);
                if (confirmation[0] == 'o' || confirmation[0] == 'O') {
                    resultat = passerSeanceSuivante(c);
                    printf("\n%s\n", messageCloture(resultat));
                    if (resultat == CLOTURE_OK) {
                        if (!sauvegarderTout(t))
                            fprintf(stderr, "Erreur : sauvegarde impossible.\n");
                        if (c->etat == CYCLE_EN_COURS) {
                            printf("La seance %d est maintenant ouverte.\n",
                                   c->numeroSeanceActuelle);
                            printf("Beneficiaire : %s\n",
                                   nomDuMembre(&t->membres,
                                     c->seances[c->numeroSeanceActuelle - 1]
                                         .idBeneficiaire));
                        } else {
                            printf("Le cycle %d est termine.\n", c->idCycle);
                        }
                    }
                }
                break;
        }
    } while (choix != 0);
}

void menuCaisse(Tontine *t)
{
    Cycle *c;
    int choix, numeroSeance;
    if (!t || !(c = choisirCycle(t, 0)))
        return;

    do {
        afficherTitre("GESTION DE LA CAISSE");
        printf("Cycle : %d\n\n", c->idCycle);
        puts("1. Afficher la caisse actuelle");
        puts("2. Afficher l'evolution de la caisse");
        puts("3. Afficher le solde apres une seance");
        puts("4. Afficher le solde final");
        puts("0. Retour");
        printf("Votre choix : ");
        scanf("%d", &choix);
        switch (choix) {
            case 1:
                afficherCaisse(c);
                break;
            case 2:
                afficherEvolutionCaisse(c);
                break;
            case 3:
                printf("Numero de la seance : ");
                scanf("%d", &numeroSeance);
                afficherSoldeApresSeance(c, numeroSeance);
                break;
            case 4:
                afficherSoldeFinal(c);
                break;
        }
    } while (choix != 0);
}
