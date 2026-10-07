#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "utils.h"

void dateDuJour(char *destination)
{
    time_t maintenant = time(NULL);
    struct tm *dateLocale;

    if (destination == NULL)
    {
        fprintf(stderr, "Erreur : destination de date invalide.\n");
        return;
    }

    dateLocale = localtime(&maintenant);
    if (dateLocale == NULL ||
        strftime(destination, TAILLE_DATE, "%d/%m/%Y", dateLocale) == 0)
    {
        destination[0] = '\0';
        fprintf(stderr, "Erreur : impossible de determiner la date du jour.\n");
    }
}

void ajouterJoursADate(const char *dateDepart, int nbJours, char *dateResultat)
{
    struct tm date;
    int jour;
    int mois;
    int annee;
    time_t instant;

    if (dateResultat == NULL || dateDepart == NULL)
    {
        fprintf(stderr, "Erreur : date de depart invalide.\n");
        return;
    }

    if (sscanf(dateDepart, "%d/%d/%d", &jour, &mois, &annee) != 3)
    {
        fprintf(stderr, "Erreur : date de depart invalide.\n");
        return;
    }

    memset(&date, 0, sizeof(date));
    date.tm_mday = (int)jour;
    date.tm_mon = (int)mois - 1;
    date.tm_year = (int)annee - 1900;
    date.tm_hour = 12;
    date.tm_isdst = -1;
    instant = mktime(&date);
    if (instant == (time_t)-1)
    {
        fprintf(stderr, "Erreur : impossible de calculer la date resultat.\n");
        return;
    }
    date.tm_mday += nbJours;
    instant = mktime(&date);
    if (instant == (time_t)-1 ||
        strftime(dateResultat, TAILLE_DATE, "%d/%m/%Y", &date) == 0)
    {
        fprintf(stderr, "Erreur : impossible de calculer la date resultat.\n");
        return;
    }
}

void afficherTitre(const char *titre)
{
    printf("\n========================================\n%s\n"
           "========================================\n",
           titre != NULL ? titre : "");
}

void initialiserAleatoire(void)
{
    srand((unsigned int)time(NULL));
}

int nombreAleatoire(int min, int max)
{
    unsigned int largeur;

    if (min > max)
    {
        fprintf(stderr, "Erreur : intervalle aleatoire invalide.\n");
        return min;
    }
    largeur = (unsigned int)((long long)max - min + 1);
    if (largeur == 0 || largeur > (unsigned int)RAND_MAX)
    {
        return min + rand() % (max - min + 1);
    }
    return min + (int)((unsigned int)rand() % largeur);
}


const char *libelleEtatCycle(EtatCycle etat)
{
    switch (etat)
    {
        case CYCLE_PLANIFIE: return "PLANIFIE";
        case CYCLE_EN_COURS: return "EN COURS";
        case CYCLE_TERMINE:  return "TERMINE";
    }
    return "INCONNU";
}

const char *libelleEtatSeance(EtatSeance etat)
{
    switch (etat)
    {
        case SEANCE_OUVERTE:  return "OUVERTE";
        case SEANCE_CLOTUREE: return "CLOTUREE";
    }
    return "INCONNU";
}

const char *libelleEtatCotisation(EtatCotisation etat)
{
    switch (etat)
    {
        case COTISATION_NON_PAYEE: return "NON PAYEE";
        case COTISATION_PARTIELLE: return "PARTIELLE";
        case COTISATION_PAYEE:     return "PAYEE";
    }
    return "INCONNU";
}

const char *libelleEtatDette(EtatDette etat)
{
    switch (etat)
    {
        case DETTE_EN_COURS:           return "EN COURS";
        case DETTE_PARTIELLEMENT_REGLEE: return "PARTIELLEMENT REGLEE";
        case DETTE_REGULARISEE:        return "REGULARISEE";
    }
    return "INCONNU";
}

const char *libelleTypePenalite(TypePenalite type)
{
    switch (type)
    {
        case PENALITE_PARTIELLE:    return "PARTIELLE (10%)";
        case PENALITE_NON_PAIEMENT: return "NON PAIEMENT (15%)";
    }
    return "INCONNU";
}