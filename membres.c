#include<stdio.h>
#include<assert.h>
#include<stdlib.h>
#include<string.h>
#include"membres.h"


void initialiserListeMembres(ListeMembres *liste){
    liste->tete = NULL;
    liste->taille = 0;
}



Membre *creer_membre(int idMembre,char nom[TAILLE_NOM],char telephone[TAILLE_TELEPHONE],char lieu_de_residence[TAILLE_LIEU]){
        Membre *m=malloc(sizeof(Membre));
        assert(m!=NULL);
        m->idMembre=idMembre;
       strcpy(m->nom,nom);
       strcpy(m->telephone,telephone);
       strcpy(m->lieu_de_residence,lieu_de_residence);
       m->suivant=NULL;
       return m;
}


int insererMembre(ListeMembres *liste, Membre *membre){
    if(liste->tete==NULL){
        liste->tete = membre;
        membre->suivant=liste->tete;
    }else{
        Membre *courant=liste->tete;
        for(int i = 0; i < liste->taille; i++){
            courant = courant->suivant;
        }

        courant->suivant=membre;
        membre->suivant=liste->tete;
    }
    liste->taille++;
    return 0;
}

Membre *trouverMembreParId(const ListeMembres *liste, int idMembre){
    if(liste->tete==NULL){// j implemente le do while et pas le while habituel car on a a faire a une liste chainee circulaire car le dernier element de la chaine vas pointer sur le premier donc il ya pas de null sauf si la liste est nulle
        return NULL;
    }
     Membre *courant=liste->tete;
    do{
        if(courant->idMembre==idMembre){
            return courant;
        }else{
            courant=courant->suivant;
        }

    }while(courant!=liste->tete); 
    return NULL;
}

int idMembreExiste(const ListeMembres *liste, int idMembre){
    if(liste->tete==NULL){// j implemente le do while et pas le while habituel car on a a faire a une liste chainee circulaire car le dernier element de la chaine vas pointer sur le premier donc il ya pas de null sauf si la liste est nulle
        return 0;    }
     Membre *courant=liste->tete;
    do{
        if(courant->idMembre==idMembre){
            return 1;
        }else{
            courant=courant->suivant;
        }

    }while(courant!=liste->tete); 
    return 0;
}

int existeAuMoinsUnMembre(const ListeMembres *liste){
    if(liste->tete==NULL){// j implemente le do while et pas le while habituel car on a a faire a une liste chainee circulaire car le dernier element de la chaine vas pointer sur le premier donc il ya pas de null sauf si la liste est nulle
        return 0;    
    }else{
        return 1;
    }
}

const char *nomDuMembre(const ListeMembres *liste, int idMembre){
    Membre *membre=trouverMembreParId(liste,idMembre);
    if(membre==NULL){
        return "inconnu";
    }
    return membre->nom;
}

void libererListeMembres(ListeMembres *liste){
    if(liste->tete==NULL){
        return;
    }
    Membre *courant=liste->tete;
    for(int i=0;i<liste->taille;i++){
        Membre *suivant = courant->suivant;
        free(courant);
        courant=suivant;
    }
    liste->tete = NULL;
    liste->taille=0;
}

void afficherTousLesMembres(const ListeMembres *liste){
    printf("liste de tous les memebres du fichiers\n");
    if(liste->tete==NULL){// j implemente le do while et pas le while habituel car on a a faire a une liste chainee circulaire car le dernier element de la chaine vas pointer sur le premier donc il ya pas de null sauf si la liste est nulle
        printf("il nya aucun membre present\n");
        return;
    }
    Membre *courant=liste->tete;
    for(int i = 0; i<liste->taille; i++){
        printf("identifiant: %d\t nom: %s\t telephone: %s\t  lieu de residence: %s\n",courant->idMembre,courant->nom,courant->telephone,courant->lieu_de_residence);
        courant=courant->suivant;
    }
}

void menuMembres(Tontine *t){
    char nom[TAILLE_NOM], telephone[TAILLE_TELEPHONE], residence[TAILLE_LIEU];
    int choix,id;
    do{
        printf("\n========== Membre ==========\n");
        printf("1. ajouter un membre\n");
        printf("2. modifier un membre\n");
        printf("3. rechercher un membre\n");
        printf("4. afficher les membres\n");
        printf("5. supprimer un membre\n");
        printf("0. Quitter\n");
        printf("===============================\n");

        printf("Votre choix : ");
        scanf("%d", &choix);
        switch (choix){
        case 1:
                printf("entrer les informations du nouveau memebre ");
                printf("entrer son id");
                scanf("%d",&id);
                printf("entrer son nom ");
                scanf("%s",nom);
                printf("entrer le numero de telephone");
                scanf("%s",telephone);
                printf("entrer son lieu de residence");
                scanf("%s",&residence);
                Membre *m = creer_membre(id, nom, telephone,residence);
                ListeMembres *l = malloc(sizeof *l);
                initialiserListeMembres(l);
                insererMembre(l, m);
                
            case 2:
                /* Sous-menu des cycles */
                break;

            case 3:
                printf("\nGestion des cotisations...\n");
                break;

            case 4:
                printf("\nGestion de la caisse...\n");
                break;
        
            case 5:
                printf("\nAu revoir !\n");
                break;

            default:
                printf("\nChoix invalide.\n");
        }

    } while(choix != 5);
}