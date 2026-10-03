/* ============================================================
   FICHIERS.C - VERSION "MEMBRES SEULEMENT"
   Responsable : Personne 5

   Cette version contient uniquement ce qui sert aux membres :
     - démarrage et arrêt de la tontine ;
     - sauvegarde et chargement de membres.txt ;
     - ajouterHistorique() (utilisée par le module membres).

   Les parties "cycles" (file.txt, seance.txt, cotisation.txt,
   soldecaisse.txt) et l'affichage de l'historique sont marquées
   "À ÉCRIRE PLUS TARD". Elles sont vides pour l'instant afin que
   le programme compile et que l'on puisse tester les membres.

   ATTENTION : tant que sauvegarderCycles() et chargerCycles()
   sont vides, les cycles ne sont PAS sauvegardés.
   ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "fichiers.h"
#include "membres.h"
#include "cycles.h"
#include "utils.h"

#define TAILLE_LIGNE  300


/* ============================================================
   1. DEMARRAGE ET ARRET
   ============================================================ */

/* Met la tontine à zéro : aucun membre, aucun cycle. */
void initialiserTontine(Tontine *t)
{
    initialiserListeMembres(&t->membres);
    t->cycles = NULL;
    t->nombreCycles = 0;
}

/* Libère toute la mémoire (membres + cycles). */
void libererTontine(Tontine *t)
{
    libererListeMembres(&t->membres);
    libererTousLesCycles(t);
    t->cycles = NULL;
    t->nombreCycles = 0;
}


/* ============================================================
   2. MEMBRES : membres.txt
   Une ligne par membre :  idMembre;nom;telephone;lieu_de_residence
   ============================================================ */

/*
 * Découpe une ligne en champs séparés par ';'.
 * Chaque appel retourne le champ suivant (qui peut être vide) et
 * avance *reste. Retourne NULL quand il n'y a plus de champ.
 * (Contrairement à strtok, elle gère bien les champs vides ";;".)
 */
static char *champSuivant(char **reste)
{
    char *debut = *reste;
    char *separateur;

    if (debut == NULL)
    {
        return NULL;
    }

    separateur = strchr(debut, ';');
    if (separateur != NULL)
    {
        *separateur = '\0';             /* on coupe le champ ici */
        *reste = separateur + 1;        /* le prochain champ commence après */
    }
    else
    {
        *reste = NULL;                  /* c'était le dernier champ */
    }
    return debut;
}

/* Écrit tous les membres dans membres.txt. Retourne 1 si OK, 0 sinon. */
int sauvegarderMembres(const Tontine *t)
{
    FILE *f;
    const Membre *courant;
    int i;

    f = fopen(FICHIER_MEMBRES, "w");
    if (f == NULL)
    {
        return 0;
    }

    courant = t->membres.tete;
    for (i = 0; i < t->membres.taille; i++)     /* liste circulaire : on compte */
    {
        fprintf(f, "%d;%s;%s;%s\n",
                courant->idMembre, courant->nom,
                courant->telephone, courant->lieu_de_residence);
        courant = courant->suivant;
    }

    fclose(f);
    return 1;
}

/*
 * Lit membres.txt et remplit la liste des membres.
 * - fichier absent (premier lancement) : retourne 1, liste vide ;
 * - ligne incorrecte ou identifiant en double : retourne 0
 *   (main.c arrête alors le programme, pour ne rien écraser).
 */
int chargerMembres(Tontine *t)
{
    FILE *f;
    char ligne[TAILLE_LIGNE];
    char *reste;
    char *champId;
    char *champNom;
    char *champTel;
    char *champLieu;
    int id;
    Membre *m;

    f = fopen(FICHIER_MEMBRES, "r");
    if (f == NULL)
    {
        return (errno == ENOENT);       /* 1 si le fichier n'existe pas encore */
    }

    while (fgets(ligne, sizeof(ligne), f) != NULL)
    {
        ligne[strcspn(ligne, "\r\n")] = '\0';   /* enlève la fin de ligne */
        if (ligne[0] == '\0')
        {
            continue;                           /* ligne vide : on l'ignore */
        }

        reste = ligne;
        champId   = champSuivant(&reste);
        champNom  = champSuivant(&reste);
        champTel  = champSuivant(&reste);
        champLieu = champSuivant(&reste);

        if (champId == NULL || champNom == NULL ||
            champTel == NULL || champLieu == NULL)
        {
            fclose(f);
            return 0;                           /* ligne incomplète */
        }

        id = atoi(champId);
        if (id <= 0)
        {
            fclose(f);
            return 0;                           /* identifiant invalide */
        }

        m = creerMembre(id, champNom, champTel, champLieu);
        if (m == NULL)
        {
            fclose(f);
            return 0;                           /* plus de mémoire */
        }

        if (insererMembre(&t->membres, m) == 0)
        {
            free(m);                            /* identifiant en double */
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    return 1;
}


/* ============================================================
   3. CYCLES : À ÉCRIRE PLUS TARD
   (file.txt, seance.txt, cotisation.txt, soldecaisse.txt)
   ============================================================ */

int sauvegarderCycles(const Tontine *t)
{
    (void)t;
    return 1;       /* À ÉCRIRE PLUS TARD */
}

int chargerCycles(Tontine *t)
{
    (void)t;
    return 1;       /* À ÉCRIRE PLUS TARD */
}


/* ============================================================
   4. SAUVEGARDE ET CHARGEMENT COMPLETS
   ============================================================ */

int sauvegarderTout(const Tontine *t)
{
    int ok = 1;

    if (!sauvegarderMembres(t))
    {
        ok = 0;
    }
    if (!sauvegarderCycles(t))
    {
        ok = 0;
    }
    return ok;
}

int chargerTout(Tontine *t)
{
    if (!chargerMembres(t))
    {
        return 0;
    }
    if (!chargerCycles(t))
    {
        return 0;
    }
    return 1;
}


/* ============================================================
   5. HISTORIQUE
   ============================================================ */

/*
 * Ajoute une ligne à la fin de historique.txt :
 *     JJ/MM/AAAA;idCycle;numeroSeance;message
 * (idCycle = 0 pour un événement qui n'est lié à aucun cycle,
 *  par exemple l'ajout d'un membre.)
 */
void ajouterHistorique(int idCycle, int numeroSeance, const char *message)
{
    FILE *f;
    char date[TAILLE_DATE];

    f = fopen(FICHIER_HISTORIQUE, "a");         /* "a" = ajouter à la fin */
    if (f == NULL)
    {
        return;
    }

    dateDuJour(date);
    fprintf(f, "%s;%d;%d;%s\n", date, idCycle, numeroSeance, message);
    fclose(f);
}

/* À ÉCRIRE PLUS TARD : affichage de l'historique. */
void afficherHistorique(int idCycle)
{
    (void)idCycle;
    printf("\n  L'affichage de l'historique n'est pas encore disponible.\n");
    printf("  (Les événements sont déjà enregistrés dans historique.txt)\n\n");
}

/* À ÉCRIRE PLUS TARD : sous-menu de l'historique. */
void menuHistorique(Tontine *t)
{
    (void)t;
    afficherHistorique(0);
}