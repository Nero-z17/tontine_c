#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "utils.h"

static int lireLigne(const char *message, char *destination, size_t taille)
{
    size_t longueur;
    int caractere;

    if (message != NULL)
    {
        fputs(message, stdout);
        fflush(stdout);
    }

    if (fgets(destination, (int)taille, stdin) == NULL)
    {
        return 0;
    }

    longueur = strlen(destination);
    if (longueur > 0 && destination[longueur - 1] == '\n')
    {
        destination[--longueur] = '\0';
    }
    else
    {
        while ((caractere = getchar()) != '\n' && caractere != EOF)
        {
            if (!isspace((unsigned char)caractere))
            {
                while (caractere != '\n' && caractere != EOF)
                {
                    caractere = getchar();
                }
                printf("Saisie trop longue. Recommencez.\n");
                return -1;
            }
        }
    }

    while (longueur > 0 &&
           isspace((unsigned char)destination[longueur - 1]))
    {
        destination[--longueur] = '\0';
    }
    return 1;
}

void viderBuffer(void)
{
    int caractere;

    while ((caractere = getchar()) != '\n' && caractere != EOF)
    {
    }
}

void attendreEntree(void)
{
    char ligne[2];

    fputs("Appuyez sur Entree pour continuer...", stdout);
    fflush(stdout);
    (void)fgets(ligne, sizeof(ligne), stdin);
    if (ligne[0] != '\n' && ligne[0] != '\0')
    {
        viderBuffer();
    }
}

int lireEntier(const char *message, int min, int max)
{
    char ligne[128];
    char *fin;
    long valeur;
    int resultat;

    for (;;)
    {
        resultat = lireLigne(message, ligne, sizeof(ligne));
        if (resultat == 0)
        {
            return min;
        }
        if (resultat < 0)
        {
            continue;
        }

        errno = 0;
        valeur = strtol(ligne, &fin, 10);
        while (isspace((unsigned char)*fin))
        {
            fin++;
        }
        if (ligne[0] != '\0' && *fin == '\0' && errno != ERANGE &&
            valeur >= min && valeur <= max)
        {
            return (int)valeur;
        }
        printf("Veuillez entrer un entier entre %d et %d.\n", min, max);
    }
}

void lireChaine(const char *message, char *destination, int taille)
{
    int resultat;
    size_t i;

    if (destination == NULL || taille <= 0)
    {
        fprintf(stderr, "Erreur : destination de saisie invalide.\n");
        return;
    }

    do
    {
        resultat = lireLigne(message, destination, (size_t)taille);
        if (resultat == 0)
        {
            destination[0] = '\0';
            return;
        }
    } while (resultat < 0);

    for (i = 0; destination[i] != '\0'; i++)
    {
        if (destination[i] == ';')
        {
            destination[i] = ',';
        }
    }
}

void lireDate(const char *message, char *destination)
{
    for (;;)
    {
        lireChaine(message, destination, TAILLE_DATE);
        if (dateEstValide(destination))
        {
            return;
        }
        printf("Date invalide. Utilisez le format JJ/MM/AAAA.\n");
    }
}

int confirmer(const char *message)
{
    char ligne[32];

    for (;;)
    {
        int resultat = lireLigne(message, ligne, sizeof(ligne));
        if (resultat == 0)
        {
            return 0;
        }
        if (resultat < 0)
        {
            continue;
        }
        if (strcmp(ligne, "o") == 0 || strcmp(ligne, "O") == 0 ||
            strcmp(ligne, "oui") == 0 || strcmp(ligne, "OUI") == 0)
        {
            return 1;
        }
        if (strcmp(ligne, "n") == 0 || strcmp(ligne, "N") == 0 ||
            strcmp(ligne, "non") == 0 || strcmp(ligne, "NON") == 0)
        {
            return 0;
        }
        printf("Repondez par oui ou non.\n");
    }
}

int dateEstValide(const char *date)
{
    int jour;
    int mois;
    int annee;
    int joursParMois[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    size_t i;

    if (date == NULL || strlen(date) != 10 ||
        date[2] != '/' || date[5] != '/')
    {
        return 0;
    }
    for (i = 0; i < 10; i++)
    {
        if (i != 2 && i != 5 && !isdigit((unsigned char)date[i]))
        {
            return 0;
        }
    }

    jour = (date[0] - '0') * 10 + date[1] - '0';
    mois = (date[3] - '0') * 10 + date[4] - '0';
    annee = (date[6] - '0') * 1000 + (date[7] - '0') * 100 +
            (date[8] - '0') * 10 + date[9] - '0';

    if (mois < 1 || mois > 12 || annee < 1)
    {
        return 0;
    }
    if (mois == 2 &&
        (annee % 400 == 0 || (annee % 4 == 0 && annee % 100 != 0)))
    {
        joursParMois[1] = 29;
    }
    return jour >= 1 && jour <= joursParMois[mois - 1];
}

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
    char copie[TAILLE_DATE];
    char *fin;
    long jour;
    long mois;
    long annee;
    time_t instant;

    if (dateResultat == NULL || !dateEstValide(dateDepart))
    {
        fprintf(stderr, "Erreur : date de depart invalide.\n");
        return;
    }

    strcpy(copie, dateDepart);
    jour = strtol(copie, &fin, 10);
    mois = strtol(fin + 1, &fin, 10);
    annee = strtol(fin + 1, NULL, 10);

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