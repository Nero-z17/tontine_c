#include<stdio.h>
#include<assert.h>
#include<stdlib.h>
#include<string.h>
#include"membres.h"

#define max 40

void initialiserListeMembres(ListeMembres *liste){
    liste->tete = NULL;
    liste->taille = 0;
}



Membre *creer_membre(int idMembre,char nom[max],char telephone[max],char lieu_de_residence[max]){
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
        liste->tete->suivant=liste->tete;
        return 0;
    }else{
        Membre *courant=liste->tete;
        for(int i = 0; i < liste->taille - 1; i++){
            courant = courant->suivant;
        }

        courant->suivant=membre;
        membre->suivant=liste->tete;
    }
    return 0;
}
/*void afficherTousLesMembres(const ListeMembres *liste){
    if(liste->tete==NULL){// j implemente le do while et pas le while habituel car on a a faire a une liste chainee circulaire car le dernier element de la chaine vas pointer sur le premier donc il ya pas de null sauf si la liste est nulle
        return;
    }
    Membre *courant=tete;
    do{
        printf("identifiant: %d\t nom: %s\t telephone: %s\t  lieu de residence: %s\n",courant->idMembre,courant->nom,courant->telephone,courant->lieu_de_residence);
        courant=courant->suivant;
    }while(courant!=tete);
}*/
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
    if(liste->tete==NULL){
        char texte[20];
        strcpy(texte, "inconnu");
        return texte;
    }else{
        Membre *courant=liste->tete;
        for(int i = 0; i < liste->taille - 1; i++){
            if(courant->idMembre==idMembre){
                char texte[20];
                strcpy(texte, courant->nom);
                return texte;
            }else{
                courant=courant->suivant;
            }
        }
    char texte[20];
    strcpy(texte, "inconnu");
    return texte;
    }
}

void libererListeMembres(ListeMembres *liste){
    Membre *p = liste->tete;
    while(p!=NULL){
        Membre *suiv = p->suivant;
        free(p);
        p=suiv;
    }
    liste->tete = NULL;
}