/* ============================================================
   STUBS_TEMPORAIRES.C - Fonctions VIDES des modules pas encore écrits

   But : permettre de compiler et de lancer le programme pour tester
   seulement les membres. Chaque fonction affiche simplement
   "pas encore disponible".

   RÈGLE : dès qu'un vrai module est écrit (cycles.c, seances.c,
   cotisations.c), SUPPRIME ici ses fonctions. Sinon le compilateur
   dira "multiple definition" (définition en double).

   Ce fichier ne sera PAS dans le projet final.
   ============================================================ */

#include <stdio.h>

#include "structures.h"
#include "cycles.h"
#include "seances.h"
#include "cotisations.h"


/* ---------- module cycles (Personne 2) ---------- */

void menuCycles(Tontine *t)
{
    (void)t;
    printf("\n  Gestion des cycles : pas encore disponible.\n\n");
}

void menuBilans(Tontine *t)
{
    (void)t;
    printf("\n  Bilans : pas encore disponible.\n\n");
}

void libererTousLesCycles(Tontine *t)
{
    (void)t;                    /* aucun cycle n'existe pour l'instant */
}

int membreAUnCycleNonTermine(const Tontine *t, int idMembre)
{
    (void)t;
    (void)idMembre;
    return 0;                   /* aucun cycle : la suppression est toujours permise */
}


/* ---------- module seances (Personne 3) ---------- */

void menuSeances(Tontine *t)
{
    (void)t;
    printf("\n  Gestion des séances : pas encore disponible.\n\n");
}

void menuCaisse(Tontine *t)
{
    (void)t;
    printf("\n  Gestion de la caisse : pas encore disponible.\n\n");
}


/* ---------- module cotisations (Personne 4) ---------- */

void menuCotisations(Tontine *t)
{
    (void)t;
    printf("\n  Gestion des cotisations : pas encore disponible.\n\n");
}
