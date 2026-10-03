#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "membres.h"
#include "cycles.h"
#include "fichiers.h"
#include "utils.h"

static int chaineNonVide(const char *texte)
{
    if (texte == NULL)
    {
        return 0;
    }

    while (*texte != '\0')
    {
        if (!isspace((unsigned char)*texte))
        {
            return 1;
        }
        texte++;
    }
    return 0;
}

static int champValide(const char *texte, size_t taille)
{
    return chaineNonVide(texte) && strlen(texte) < taille;
}

static int contientSansCasse(const char *texte, const char *recherche)
{
    const char *debut;
    const char *a;
    const char *b;

    if (texte == NULL || recherche == NULL)
    {
        return 0;
    }
    if (*recherche == '\0')
    {
        return 1;
    }

    for (debut = texte; *debut != '\0'; debut++)
    {
        a = debut;
        b = recherche;
        while (*a != '\0' && *b != '\0' &&
               tolower((unsigned char)*a) == tolower((unsigned char)*b))
        {
            a++;
            b++;
        }
        if (*b == '\0')
        {
            return 1;
        }
    }
    return 0;
}

static void enregistrerOperation(const Tontine *t, const char *message)
{
    if (!sauvegarderTout(t))
    {
        printf("Erreur : impossible de sauvegarder les donnees des membres.\n");
    }
    ajouterHistorique(0, 0, message);
}

void initialiserListeMembres(ListeMembres *liste)
{
    if (liste != NULL)
    {
        liste->tete = NULL;
        liste->taille = 0;
    }
}

Membre *creerMembre(int idMembre, const char *nom,
                    const char *telephone, const char *lieu_de_residence)
{
    Membre *membre;

    if (idMembre <= 0 ||
        !champValide(nom, TAILLE_NOM) ||
        !champValide(telephone, TAILLE_TELEPHONE) ||
        !champValide(lieu_de_residence, TAILLE_LIEU))
    {
        return NULL;
    }

    membre = malloc(sizeof(*membre));
    if (membre == NULL)
    {
        return NULL;
    }

    membre->idMembre = idMembre;
    strcpy(membre->nom, nom);
    strcpy(membre->telephone, telephone);
    strcpy(membre->lieu_de_residence, lieu_de_residence);
    membre->suivant = NULL;
    return membre;
}

int insererMembre(ListeMembres *liste, Membre *membre)
{
    Membre *dernier;

    if (liste == NULL || membre == NULL || membre->suivant != NULL ||
        membre->idMembre <= 0 || idMembreExiste(liste, membre->idMembre))
    {
        return 0;
    }

    if (liste->tete == NULL)
    {
        liste->tete = membre;
        membre->suivant = membre;
    }
    else
    {
        dernier = liste->tete;
        while (dernier->suivant != liste->tete)
        {
            dernier = dernier->suivant;
        }
        dernier->suivant = membre;
        membre->suivant = liste->tete;
    }
    liste->taille++;
    return 1;
}

Membre *trouverMembreParId(const ListeMembres *liste, int idMembre)
{
    Membre *courant;

    if (liste == NULL || liste->tete == NULL)
    {
        return NULL;
    }

    courant = liste->tete;
    do
    {
        if (courant->idMembre == idMembre)
        {
            return courant;
        }
        courant = courant->suivant;
    } while (courant != liste->tete);

    return NULL;
}

int idMembreExiste(const ListeMembres *liste, int idMembre)
{
    return trouverMembreParId(liste, idMembre) != NULL;
}

int existeAuMoinsUnMembre(const ListeMembres *liste)
{
    return liste != NULL && liste->tete != NULL;
}

const char *nomDuMembre(const ListeMembres *liste, int idMembre)
{
    Membre *membre = trouverMembreParId(liste, idMembre);
    return membre != NULL ? membre->nom : "Inconnu";
}

void libererListeMembres(ListeMembres *liste)
{
    Membre *courant;
    Membre *suivant;
    Membre *dernier;

    if (liste == NULL || liste->tete == NULL)
    {
        if (liste != NULL)
        {
            liste->taille = 0;
        }
        return;
    }

    dernier = liste->tete;
    while (dernier->suivant != liste->tete)
    {
        dernier = dernier->suivant;
    }
    dernier->suivant = NULL;

    courant = liste->tete;
    while (courant != NULL)
    {
        suivant = courant->suivant;
        free(courant);
        courant = suivant;
    }

    liste->tete = NULL;
    liste->taille = 0;
}

int ajouterMembre(Tontine *t, int idMembre, const char *nom,
                  const char *telephone, const char *lieu_de_residence)
{
    Membre *membre;

    if (t == NULL || idMembre <= 0 ||
        idMembreExiste(&t->membres, idMembre) ||
        !champValide(nom, TAILLE_NOM) ||
        !champValide(telephone, TAILLE_TELEPHONE) ||
        !champValide(lieu_de_residence, TAILLE_LIEU))
    {
        return 0;
    }

    membre = creerMembre(idMembre, nom, telephone, lieu_de_residence);
    if (membre == NULL)
    {
        return 0;
    }
    if (!insererMembre(&t->membres, membre))
    {
        free(membre);
        return 0;
    }
    return 1;
}

int modifierMembre(Tontine *t, int idMembre, const char *nouveauNom,
                   const char *nouveauTelephone, const char *nouveauLieu)
{
    Membre *membre;

    if (t == NULL ||
        !champValide(nouveauNom, TAILLE_NOM) ||
        !champValide(nouveauTelephone, TAILLE_TELEPHONE) ||
        !champValide(nouveauLieu, TAILLE_LIEU))
    {
        return 0;
    }

    membre = trouverMembreParId(&t->membres, idMembre);
    if (membre == NULL)
    {
        return 0;
    }

    strcpy(membre->nom, nouveauNom);
    strcpy(membre->telephone, nouveauTelephone);
    strcpy(membre->lieu_de_residence, nouveauLieu);
    return 1;
}

ResultatSuppression supprimerMembre(Tontine *t, int idMembre)
{
    Membre *courant;
    Membre *precedent;

    if (t == NULL || t->membres.tete == NULL)
    {
        return SUPPRESSION_MEMBRE_INTROUVABLE;
    }
    if (membreAUnCycleNonTermine(t, idMembre))
    {
        return SUPPRESSION_REFUSEE_CYCLE_NON_TERMINE;
    }

    courant = t->membres.tete;
    while (courant->idMembre != idMembre)
    {
        courant = courant->suivant;
        if (courant == t->membres.tete)
        {
            return SUPPRESSION_MEMBRE_INTROUVABLE;
        }
    }

    if (courant->suivant == courant)
    {
        t->membres.tete = NULL;
    }
    else
    {
        if (courant == t->membres.tete)
        {
            precedent = t->membres.tete;
            while (precedent->suivant != courant)
            {
                precedent = precedent->suivant;
            }
            precedent->suivant = courant->suivant;
            t->membres.tete = courant->suivant;
        }
        else
        {
            precedent = t->membres.tete;
            while (precedent->suivant != courant)
            {
                precedent = precedent->suivant;
            }
            precedent->suivant = courant->suivant;
        }
    }
    free(courant);
    t->membres.taille--;
    return SUPPRESSION_OK;
}

const char *messageSuppression(ResultatSuppression resultat)
{
    switch (resultat)
    {
        case SUPPRESSION_OK:
            return "Le membre a ete supprime.";
        case SUPPRESSION_MEMBRE_INTROUVABLE:
            return "Aucun membre ne correspond a cet identifiant.";
        case SUPPRESSION_REFUSEE_CYCLE_NON_TERMINE:
            return "Suppression refusee : le membre participe a un cycle non termine.";
        default:
            return "Resultat de suppression inconnu.";
    }
}

void afficherMembre(const Membre *membre)
{
    if (membre == NULL)
    {
        printf("Membre introuvable.\n");
        return;
    }
    printf("Identifiant : %d | Nom : %s | Telephone : %s | Residence : %s\n",
           membre->idMembre, membre->nom, membre->telephone,
           membre->lieu_de_residence);
}

void afficherTousLesMembres(const ListeMembres *liste)
{
    Membre *courant;
    int i;

    if (!existeAuMoinsUnMembre(liste))
    {
        printf("Aucun membre n'est enregistre.\n");
        return;
    }

    printf("\nListe des membres (%d) :\n", liste->taille);
    courant = liste->tete;
    for (i = 0; i < liste->taille; i++)
    {
        afficherMembre(courant);
        courant = courant->suivant;
    }
}

int afficherMembresParNom(const ListeMembres *liste, const char *texte)
{
    Membre *courant;
    int i;
    int nombreTrouve = 0;

    if (!existeAuMoinsUnMembre(liste) || texte == NULL)
    {
        printf("Aucun membre ne correspond a cette recherche.\n");
        return 0;
    }

    courant = liste->tete;
    for (i = 0; i < liste->taille; i++)
    {
        if (contientSansCasse(courant->nom, texte))
        {
            afficherMembre(courant);
            nombreTrouve++;
        }
        courant = courant->suivant;
    }
    if (nombreTrouve == 0)
    {
        printf("Aucun membre ne correspond a cette recherche.\n");
    }
    return nombreTrouve;
}

void menuMembres(Tontine *t)
{
    char nom[TAILLE_NOM];
    char telephone[TAILLE_TELEPHONE];
    char residence[TAILLE_LIEU];
    char recherche[TAILLE_NOM];
    char historique[TAILLE_MESSAGE_HISTORIQUE];
    int choix;
    int id;
    int typeRecherche;
    Membre *membre;
    ResultatSuppression resultat;

    if (t == NULL)
    {
        printf("Erreur : les donnees de la tontine sont indisponibles.\n");
        return;
    }

    do
    {
        afficherTitre("GESTION DES MEMBRES");
        printf("1. Ajouter un membre\n");
        printf("2. Modifier un membre\n");
        printf("3. Rechercher un membre\n");
        printf("4. Afficher tous les membres\n");
        printf("5. Supprimer un membre\n");
        printf("0. Retour\n");

        choix = lireEntier("\nVotre choix : ", 0, 5);
        switch (choix)
        {
            case 1:
                id = lireEntier("Identifiant du nouveau membre : ", 1, 2147483647);
                lireChaine("Nom : ", nom, TAILLE_NOM);
                lireChaine("Telephone : ", telephone, TAILLE_TELEPHONE);
                lireChaine("Lieu de residence : ", residence, TAILLE_LIEU);
                if (ajouterMembre(t, id, nom, telephone, residence))
                {
                    printf("Membre ajoute avec succes.\n");
                    snprintf(historique, sizeof(historique),
                             "Ajout du membre %d (%s)", id, nom);
                    enregistrerOperation(t, historique);
                }
                else
                {
                    printf("Ajout impossible : identifiant deja utilise, "
                           "champ vide ou trop long, ou memoire insuffisante.\n");
                }
                break;

            case 2:
                id = lireEntier("Identifiant du membre a modifier : ", 1, 2147483647);
                membre = trouverMembreParId(&t->membres, id);
                if (membre == NULL)
                {
                    printf("Aucun membre ne correspond a cet identifiant.\n");
                    break;
                }
                lireChaine("Nouveau nom : ", nom, TAILLE_NOM);
                lireChaine("Nouveau telephone : ", telephone, TAILLE_TELEPHONE);
                lireChaine("Nouveau lieu de residence : ", residence, TAILLE_LIEU);
                if (modifierMembre(t, id, nom, telephone, residence))
                {
                    printf("Informations du membre mises a jour.\n");
                    snprintf(historique, sizeof(historique),
                             "Modification du membre %d (%s)", id, nom);
                    enregistrerOperation(t, historique);
                }
                else
                {
                    printf("Modification impossible : un champ est vide ou trop long.\n");
                }
                break;

            case 3:
                typeRecherche = lireEntier(
                    "Rechercher par : 1. Identifiant  2. Nom\nVotre choix : ", 1, 2);
                if (typeRecherche == 1)
                {
                    id = lireEntier("Identifiant a rechercher : ", 1, 2147483647);
                    membre = trouverMembreParId(&t->membres, id);
                    if (membre == NULL)
                    {
                        printf("Aucun membre ne correspond a cet identifiant.\n");
                    }
                    else
                    {
                        afficherMembre(membre);
                    }
                }
                else
                {
                    lireChaine("Texte a rechercher dans le nom : ",
                               recherche, TAILLE_NOM);
                    afficherMembresParNom(&t->membres, recherche);
                }
                break;

            case 4:
                afficherTousLesMembres(&t->membres);
                break;

            case 5:
                id = lireEntier("Identifiant du membre a supprimer : ", 1, 2147483647);
                resultat = supprimerMembre(t, id);
                printf("%s\n", messageSuppression(resultat));
                if (resultat == SUPPRESSION_OK)
                {
                    snprintf(historique, sizeof(historique),
                             "Suppression du membre %d", id);
                    enregistrerOperation(t, historique);
                }
                break;

            case 0:
                break;
        }
    } while (choix != 0);
}
