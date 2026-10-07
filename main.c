/* ============================================================
   MAIN.C - Programme principal de la gestion d'une tontine
   Responsable : Personne 5

   Ce fichier fait seulement 4 choses :
     1. préparer le programme ;
     2. charger les données depuis les fichiers ;
     3. afficher le menu et appeler le bon sous-menu ;
     4. sauvegarder et libérer la mémoire avant de quitter.

   Toute la logique est dans les autres modules :
   chaque sous-menu est écrit dans le module concerné.
   ============================================================ */

#include <stdio.h>

#include "structures.h"
#include "utils.h"
#include "membres.h"
#include "cycles.h"
#include "seances.h"
#include "cotisations.h"
#include "fichiers.h"


int main(void)
{
    Tontine t;       /* contient TOUTES les données : membres et cycles */
    int choix;       /* le choix de l'utilisateur dans le menu */
    int aMembres;    /* 1 s'il y a au moins un membre, 0 sinon */
    int sauvegardeReussie;

    /* ---------- 1. Préparation ---------- */
    initialiserAleatoire();      /* pour le tirage au sort des bénéficiaires */
    initialiserTontine(&t);      /* tontine vide au départ */

    /* ---------- 2. Chargement des données ---------- */
    if (chargerTout(&t) == 0)
    {
        printf("Erreur : impossible de charger les fichiers de données.\n");
        libererTontine(&t);
        return 1;                /* on arrête pour ne pas écraser les fichiers */
    }

    /* ---------- 3. Menu principal ---------- */
    do
    {
        aMembres = existeAuMoinsUnMembre(&t.membres);

        afficherTitre("GESTION DE LA TONTINE");
        printf("1. Gestion des membres\n");
        printf("2. Gestion des cycles\n");
        printf("3. Gestion des seances\n");
        printf("4. Gestion des cotisations\n");
        printf("5. Gestion de la caisse\n");
        printf("6. Historique\n");
        printf("7. Bilans\n");
        printf("0. Quitter\n");

        printf("\nVotre choix : ");
        scanf("%d", &choix);

        if (aMembres == 0 && choix >= 2 && choix <= 5)
        {
            /* Sans membre, les options 2 à 5 sont bloquées */
            printf("\nAucun membre n'est enregistre. ");
            printf("Veuillez ajouter au moins un membre avant de poursuivre.\n\n");
        }
        else
        {
            switch (choix)
            {
                case 1: menuMembres(&t);      break;
                case 2: menuCycles(&t);       break;
                case 3: menuSeances(&t);      break;
                case 4: menuCotisations(&t);  break;
                case 5: menuCaisse(&t);       break;
                case 6: menuHistorique(&t);   break;
                case 7: menuBilans(&t);       break;
                case 0: break;                /* quitter : rien à faire ici */
            }

            if (!sauvegarderTout(&t))
            {
                fprintf(stderr,
                        "Erreur : impossible de sauvegarder les donnees.\n");
            }
        }

    } while (choix != 0);

    /* ---------- 4. Fin du programme ---------- */
    sauvegardeReussie = sauvegarderTout(&t);
    libererTontine(&t);              /* libère toute la mémoire */
    if (sauvegardeReussie)
    {
        printf("\nDonnees sauvegardees. Au revoir !\n");
    }
    else
    {
        fprintf(stderr,
                "\nErreur : la sauvegarde finale a echoue. "
                "Verifiez les fichiers de donnees.\n");
    }

    return sauvegardeReussie ? 0 : 1;
}