#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "fichiers.h"
#include "membres.h"
#include "cycles.h"
#include "seances.h"
#include "cotisations.h"
#include "utils.h"
#define TAILLE_LIGNE  4096
void initialiserTontine(Tontine *t) {
    initialiserListeMembres(&t->membres);
    t->cycles = NULL;
    t->nombreCycles = 0;
}
void libererTontine(Tontine *t) {
    libererListeMembres(&t->membres);
    libererTousLesCycles(t);
    t->cycles = NULL;
    t->nombreCycles = 0;
}
static char *champSuivant(char **reste) {
    char *debut = *reste;
    char *separateur;
    if (debut == NULL) return NULL;
    separateur = strchr(debut, ';');
    if (separateur != NULL) {
        *separateur = '\0';
        *reste = separateur + 1;
    } else {
        *reste = NULL;
    }
    return debut;
}
int sauvegarderMembres(const Tontine *t) {
    FILE *f;
    const Membre *courant;
    int i;
    if (t == NULL) return 0;
    f = fopen(FICHIER_MEMBRES, "w");
    if (f == NULL) return 0;
    courant = t->membres.tete;
    for (i = 0; i < t->membres.taille; i++) {
        if (courant == NULL ||
            fprintf(f, "%d;%s;%s;%s\n",
                    courant->idMembre, courant->nom,
                    courant->telephone, courant->lieu_de_residence) < 0) {
            fclose(f);
            return 0;
        }
        courant = courant->suivant;
    }
    return fclose(f) == 0;
}
int chargerMembres(Tontine *t) {
    FILE *f;
    char ligne[TAILLE_LIGNE];
    char *reste, *champId, *champNom, *champTel, *champLieu, *finId;
    ListeMembres membresChargees;
    int champsSupplementaires, id;
    long valeurId;
    Membre *m;
    if (t == NULL) return 0;
    f = fopen(FICHIER_MEMBRES, "r");
    if (f == NULL) {
        if (errno == ENOENT) {
            libererListeMembres(&t->membres);
            return 1;
        }
        return 0;
    }
    initialiserListeMembres(&membresChargees);
    while (fgets(ligne, sizeof(ligne), f) != NULL) {
        if (strchr(ligne, '\n') == NULL && !feof(f)) {
            libererListeMembres(&membresChargees);
            fclose(f);
            return 0;
        }
        ligne[strcspn(ligne, "\r\n")] = '\0';
        if (ligne[0] == '\0') continue;
        reste = ligne;
        champId   = champSuivant(&reste);
        champNom  = champSuivant(&reste);
        champTel  = champSuivant(&reste);
        champLieu = champSuivant(&reste);
        champsSupplementaires = (reste != NULL);
        if (champId == NULL || champNom == NULL ||
            champTel == NULL || champLieu == NULL || champsSupplementaires) {
            libererListeMembres(&membresChargees);
            fclose(f);
            return 0;
        }
        errno = 0;
        valeurId = strtol(champId, &finId, 10);
        if (champId[0] == '\0' || *finId != '\0' || errno == ERANGE ||
            valeurId <= 0 || valeurId > INT_MAX) {
            libererListeMembres(&membresChargees);
            fclose(f);
            return 0;
        }
        id = (int)valeurId;
        m = creerMembre(id, champNom, champTel, champLieu);
        if (m == NULL) {
            libererListeMembres(&membresChargees);
            fclose(f);
            return 0;
        }
        if (insererMembre(&membresChargees, m) == 0) {
            free(m);
            libererListeMembres(&membresChargees);
            fclose(f);
            return 0;
        }
    }
    if (ferror(f)) {
        libererListeMembres(&membresChargees);
        fclose(f);
        return 0;
    }
    if (fclose(f) != 0) {
        libererListeMembres(&membresChargees);
        return 0;
    }
    libererListeMembres(&t->membres);
    t->membres = membresChargees;
    return 1;
}
static int ecrireCycles(FILE *cycles, FILE *seances, FILE *cotisations,
                        FILE *soldes, const Tontine *t) {
    const Cycle *c;
    const Cotisation *x;
    const Dette *d;
    const Paiement *p;
    int i;
    for (c = t->cycles; c; c = c->suivant) {
        if (c->nombreParticipants < 0 ||
            (c->nombreParticipants && (!c->participants || !c->seances)))
            return 0;
        if (fprintf(cycles, "CYCLE;%d;%s;%d;%d;%d;%d;%d;%d\nORDRE;%d",
                    c->idCycle, c->dateDebut, c->frequenceJours,
                    c->montantCotisation, c->nombreParticipants,
                    c->numeroSeanceActuelle, (int)c->etat, c->soldeCaisse,
                    c->idCycle) < 0)
            return 0;
        for (i = 0; i < c->nombreParticipants; i++)
            if (fprintf(cycles, ";%d", c->participants[i]) < 0)
                return 0;
        if (fputc('\n', cycles) == EOF)
            return 0;
        for (i = 0; i < c->nombreParticipants; i++) {
            const Seance *s = &c->seances[i];
            if (fprintf(seances, "%d;%d;%s;%d;%d;%d;%d;%d;%d;%d\n",
                        s->idCycle, s->numeroSeance, s->date,
                        s->idBeneficiaire, (int)s->etat, s->montantBrut,
                        s->retenuePrincipal, s->retenuePenalites,
                        s->montantNet, s->soldeCaisseApres) < 0 ||
                fprintf(soldes, "%d;%d;%d\n", s->idCycle,
                        s->numeroSeance, s->soldeCaisseApres) < 0)
                return 0;
        }
        for (x = c->cotisations; x; x = x->suivant)
            if (fprintf(cotisations, "COTISATION;%d;%d;%d;%d;%d;%d\n",
                        x->idCycle, x->numeroSeance, x->idMembre,
                        x->montantAttendu, x->montantPaye,
                        (int)x->etat) < 0)
                return 0;
        for (d = c->dettes; d; d = d->suivant)
            if (fprintf(cotisations,
                        "DETTE;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d\n",
                        d->idCycle, d->numeroSeanceOrigine, d->idDebiteur,
                        d->idBeneficiaireOrigine, d->montantPayeInitial,
                        d->principalInitial, d->principalRestant,
                        d->nombreSeancesRetard, (int)d->typePenalite,
                        d->penaliteTotale, d->penalitePayee, (int)d->etat,
                        d->numeroSeanceRegularisation) < 0)
                return 0;
        for (p = c->paiements; p; p = p->suivant)
            if (fprintf(cotisations,
                        "PAIEMENT;%d;%d;%d;%s;%d;%d;%d;%d\n",
                        p->idCycle, p->numeroSeance, p->idMembre, p->date,
                        p->montantTotal, p->montantDette, p->montantPenalite,
                        p->montantCotisationCourante) < 0)
                return 0;
    }
    return 1;
}
int sauvegarderCycles(const Tontine *t) {
    FILE *cycles, *seances, *cotisations, *soldes;
    int ok;
    if (!t)
        return 0;
    cycles = fopen(FICHIER_FILE, "w");
    seances = fopen(FICHIER_SEANCE, "w");
    cotisations = fopen(FICHIER_COTISATION, "w");
    soldes = fopen(FICHIER_SOLDECAISSE, "w");
    if (!cycles || !seances || !cotisations || !soldes) {
        if (cycles) fclose(cycles);
        if (seances) fclose(seances);
        if (cotisations) fclose(cotisations);
        if (soldes) fclose(soldes);
        return 0;
    }
    ok = ecrireCycles(cycles, seances, cotisations, soldes, t);
    if (fclose(cycles) != 0) ok = 0;
    if (fclose(seances) != 0) ok = 0;
    if (fclose(cotisations) != 0) ok = 0;
    if (fclose(soldes) != 0) ok = 0;
    return ok;
}
static int lireChampEntier(char **reste, int *valeur) {
    char *champ = champSuivant(reste), *fin;
    long nombre;
    if (!champ || !*champ)
        return 0;
    errno = 0;
    nombre = strtol(champ, &fin, 10);
    if (errno == ERANGE || *fin || nombre < INT_MIN || nombre > INT_MAX)
        return 0;
    *valeur = (int)nombre;
    return 1;
}
static int ouvrirLecture(const char *nom, FILE **f) {
    *f = fopen(nom, "r");
    return *f != NULL || errno == ENOENT;
}
static int lireCycles(FILE *f, Tontine *t) {
    char ligne[TAILLE_LIGNE], *reste, *tag, *date;
    char dateDebut[TAILLE_DATE];
    int id, frequence, montant, nombre, actuelle, etat, solde, ordreId, i;
    Cycle *c;
    while (f && fgets(ligne, sizeof(ligne), f)) {
        if (!strchr(ligne, '\n') && !feof(f))
            return 0;
        ligne[strcspn(ligne, "\r\n")] = '\0';
        if (!ligne[0])
            continue;
        reste = ligne;
        tag = champSuivant(&reste);
        if (!tag || strcmp(tag, "CYCLE") ||
            !lireChampEntier(&reste, &id) || !(date = champSuivant(&reste)) ||
            !*date || strlen(date) >= TAILLE_DATE ||
            !lireChampEntier(&reste, &frequence) ||
            !lireChampEntier(&reste, &montant) ||
            !lireChampEntier(&reste, &nombre) ||
            !lireChampEntier(&reste, &actuelle) ||
            !lireChampEntier(&reste, &etat) ||
            !lireChampEntier(&reste, &solde) || reste ||
            id <= 0 || frequence <= 0 || montant <= 0 || nombre < 2 ||
            actuelle < 0 || actuelle > nombre ||
            etat < CYCLE_PLANIFIE || etat > CYCLE_TERMINE)
            return 0;
        strcpy(dateDebut, date);
        if (!fgets(ligne, sizeof(ligne), f) ||
            (!strchr(ligne, '\n') && !feof(f)))
            return 0;
        ligne[strcspn(ligne, "\r\n")] = '\0';
        reste = ligne;
        tag = champSuivant(&reste);
        if (!tag || strcmp(tag, "ORDRE") ||
            !lireChampEntier(&reste, &ordreId) || ordreId != id)
            return 0;
        c = nouveauCycle();
        if (!c)
            return 0;
        c->idCycle = id; strcpy(c->dateDebut, dateDebut);
        c->frequenceJours = frequence; c->montantCotisation = montant;
        c->nombreParticipants = nombre; c->numeroSeanceActuelle = actuelle;
        c->etat = (EtatCycle)etat; c->soldeCaisse = solde;
        c->participants = malloc((size_t)nombre * sizeof(*c->participants));
        if (!c->participants || !allouerSeances(c)) {
            libererCycle(c);
            return 0;
        }
        for (i = 0; i < nombre; i++)
            if (!lireChampEntier(&reste, &c->participants[i])) {
                libererCycle(c);
                return 0;
            }
        for (i = 0; i < nombre; i++) {
            int j;
            if (c->participants[i] <= 0) {
                libererCycle(c);
                return 0;
            }
            for (j = i + 1; j < nombre; j++)
                if (c->participants[i] == c->participants[j]) {
                    libererCycle(c);
                    return 0;
                }
        }
        if (reste) {
            libererCycle(c);
            return 0;
        }
        construireFile(c);
        if (c->fileBeneficiaires.taille != nombre || !ajouterCycleFin(t, c)) {
            libererCycle(c);
            return 0;
        }
        if (actuelle > 0)
            positionnerFile(c, actuelle);
    }
    return !f || !ferror(f);
}
static int lireSeances(FILE *f, Tontine *t) {
    char ligne[TAILLE_LIGNE], *reste, *date;
    int id, numero, beneficiaire, etat, brut, principal, penalites, net, solde;
    Cycle *c;
    Seance *s;
    while (f && fgets(ligne, sizeof(ligne), f)) {
        if (!strchr(ligne, '\n') && !feof(f))
            return 0;
        ligne[strcspn(ligne, "\r\n")] = '\0';
        reste = ligne;
        if (!ligne[0]) continue;
        if (!lireChampEntier(&reste, &id) ||
            !lireChampEntier(&reste, &numero) || !(date = champSuivant(&reste)) ||
            !*date || strlen(date) >= TAILLE_DATE ||
            !lireChampEntier(&reste, &beneficiaire) ||
            !lireChampEntier(&reste, &etat) ||
            !lireChampEntier(&reste, &brut) ||
            !lireChampEntier(&reste, &principal) ||
            !lireChampEntier(&reste, &penalites) ||
            !lireChampEntier(&reste, &net) ||
            !lireChampEntier(&reste, &solde) || reste ||
            etat < SEANCE_OUVERTE || etat > SEANCE_CLOTUREE ||
            !(c = trouverCycle(t, id)) || !(s = trouverSeance(c, numero)))
            return 0;
        s->idCycle = id; s->numeroSeance = numero; strcpy(s->date, date);
        s->idBeneficiaire = beneficiaire; s->etat = (EtatSeance)etat;
        s->montantBrut = brut;
        s->retenuePrincipal = principal; s->retenuePenalites = penalites;
        s->montantNet = net; s->soldeCaisseApres = solde;
    }
    return !f || !ferror(f);
}
static int lireCotisations(FILE *f, Tontine *t) {
    char ligne[TAILLE_LIGNE], *reste, *tag, *date;
    int id, a, b;
    Cycle *cycle;
    while (f && fgets(ligne, sizeof(ligne), f)) {
        if (!strchr(ligne, '\n') && !feof(f))
            return 0;
        ligne[strcspn(ligne, "\r\n")] = '\0';
        if (!ligne[0]) continue;
        reste = ligne;
        tag = champSuivant(&reste);
        if (!tag || !lireChampEntier(&reste, &id) ||
            !(cycle = trouverCycle(t, id)))
            return 0;
        if (!strcmp(tag, "COTISATION")) {
            Cotisation *x = nouvelleCotisation();
            if (!x) return 0;
            if (!lireChampEntier(&reste, &x->numeroSeance) ||
                !lireChampEntier(&reste, &x->idMembre) ||
                !lireChampEntier(&reste, &x->montantAttendu) ||
                !lireChampEntier(&reste, &x->montantPaye) ||
                !lireChampEntier(&reste, &a) || reste ||
                a < COTISATION_NON_PAYEE || a > COTISATION_PAYEE ||
                !trouverSeance(cycle, x->numeroSeance)) { free(x); return 0; }
            x->idCycle = id; x->etat = (EtatCotisation)a;
            if (!ajouterCotisationFin(cycle, x)) { free(x); return 0; }
        } else if (!strcmp(tag, "DETTE")) {
            Dette *x = nouvelleDette();
            if (!x) return 0;
            if (!lireChampEntier(&reste, &x->numeroSeanceOrigine) ||
                !lireChampEntier(&reste, &x->idDebiteur) ||
                !lireChampEntier(&reste, &x->idBeneficiaireOrigine) ||
                !lireChampEntier(&reste, &x->montantPayeInitial) ||
                !lireChampEntier(&reste, &x->principalInitial) ||
                !lireChampEntier(&reste, &x->principalRestant) ||
                !lireChampEntier(&reste, &x->nombreSeancesRetard) ||
                !lireChampEntier(&reste, &a) ||
                !lireChampEntier(&reste, &x->penaliteTotale) ||
                !lireChampEntier(&reste, &x->penalitePayee) ||
                !lireChampEntier(&reste, &b) ||
                !lireChampEntier(&reste, &x->numeroSeanceRegularisation) ||
                reste || a < PENALITE_PARTIELLE || a > PENALITE_NON_PAIEMENT ||
                b < DETTE_EN_COURS || b > DETTE_REGULARISEE ||
                !trouverSeance(cycle, x->numeroSeanceOrigine)) { free(x); return 0; }
            x->idCycle = id; x->typePenalite = (TypePenalite)a;
            x->etat = (EtatDette)b;
            if (!ajouterDetteFin(cycle, x)) { free(x); return 0; }
        } else if (!strcmp(tag, "PAIEMENT")) {
            Paiement *x = nouveauPaiement();
            if (!x) return 0;
            if (!lireChampEntier(&reste, &x->numeroSeance) ||
                !lireChampEntier(&reste, &x->idMembre) ||
                !(date = champSuivant(&reste)) || strlen(date) >= TAILLE_DATE ||
                !lireChampEntier(&reste, &x->montantTotal) ||
                !lireChampEntier(&reste, &x->montantDette) ||
                !lireChampEntier(&reste, &x->montantPenalite) ||
                !lireChampEntier(&reste, &x->montantCotisationCourante) ||
                reste || !trouverSeance(cycle, x->numeroSeance)) { free(x); return 0; }
            x->idCycle = id; strcpy(x->date, date);
            if (!ajouterPaiementFin(cycle, x)) { free(x); return 0; }
        } else {
            return 0;
        }
    }
    return !f || !ferror(f);
}
int chargerCycles(Tontine *t) {
    FILE *cycles = NULL, *seances = NULL, *cotisations = NULL;
    Tontine chargee = {0}; int ok;
    if (!t || !ouvrirLecture(FICHIER_FILE, &cycles) ||
        !ouvrirLecture(FICHIER_SEANCE, &seances) ||
        !ouvrirLecture(FICHIER_COTISATION, &cotisations)) {
        if (cycles) fclose(cycles);
        if (seances) fclose(seances);
        if (cotisations) fclose(cotisations);
        return 0;
    }
    ok = lireCycles(cycles, &chargee) && (!chargee.cycles || (seances && cotisations)) &&
         lireSeances(seances, &chargee) && lireCotisations(cotisations, &chargee);
    if (cycles && fclose(cycles) != 0) ok = 0;
    if (seances && fclose(seances) != 0) ok = 0;
    if (cotisations && fclose(cotisations) != 0) ok = 0;
    if (!ok) {
        libererTousLesCycles(&chargee);
        return 0;
    }
    libererTousLesCycles(t);
    t->cycles = chargee.cycles;
    t->nombreCycles = chargee.nombreCycles;
    return 1;
}
int sauvegarderTout(const Tontine *t) {
    int ok = 1;
    if (!sauvegarderMembres(t)) ok = 0;
    if (!sauvegarderCycles(t)) ok = 0;
    return ok;
}
int chargerTout(Tontine *t) {
    if (!chargerMembres(t)) return 0;
    if (!chargerCycles(t)) return 0;
    return 1;
}
void ajouterHistorique(int idCycle, int numeroSeance, const char *message) {
    FILE *f;
    char date[TAILLE_DATE];
    int ecritureOk;
    if (message == NULL) {
        fprintf(stderr, "Erreur : message d'historique invalide.\n");
        return;
    }
    f = fopen(FICHIER_HISTORIQUE, "a");
    if (f == NULL) {
        fprintf(stderr, "Erreur : impossible d'ouvrir %s en ecriture.\n",
                FICHIER_HISTORIQUE);
        return;
    }
    dateDuJour(date);
    ecritureOk = fprintf(f, "%s;%d;%d;%s\n",
                          date, idCycle, numeroSeance, message) >= 0;
    if (fclose(f) != 0) ecritureOk = 0;
    if (!ecritureOk) {
        fprintf(stderr, "Erreur : impossible d'ecrire dans %s.\n",
                FICHIER_HISTORIQUE);
    }
}
void afficherHistorique(int idCycle) {
    FILE *f = fopen(FICHIER_HISTORIQUE, "r");
    char ligne[TAILLE_LIGNE], date[TAILLE_DATE], message[TAILLE_MESSAGE_HISTORIQUE];
    int cycle, seance;
    if (!f) {
        if (errno == ENOENT) puts("Aucun historique n'est disponible.");
        else fprintf(stderr, "Erreur : impossible de lire %s.\n", FICHIER_HISTORIQUE);
        return;
    }
    afficherTitre(idCycle ? "HISTORIQUE DU CYCLE" : "HISTORIQUE COMPLET");
    while (fgets(ligne, sizeof(ligne), f)) {
        if (sscanf(ligne, "%10[^;];%d;%d;%199[^\r\n]", date, &cycle, &seance,
                   message) != 4) {
            fprintf(stderr, "Erreur : ligne d'historique incorrecte.\n");
            fclose(f);
            return;
        }
        if (!idCycle || cycle == idCycle)
            printf("%s;%d;%d;%s\n", date, cycle, seance, message);
    }
    if (ferror(f))
        fprintf(stderr, "Erreur : lecture de %s impossible.\n", FICHIER_HISTORIQUE);
    fclose(f);
}
void menuHistorique(Tontine *t) {
    int choix, id;
    do {
        afficherTitre("HISTORIQUE");
        puts("1. Afficher tout l'historique\n2. Afficher l'historique d'un cycle\n0. Retour");
        printf("Votre choix : "); scanf("%d", &choix);
        if (choix == 1) afficherHistorique(0); else if (choix == 2) {
            afficherTousLesCycles(t);
            printf("Identifiant du cycle : "); scanf("%d", &id);
            afficherHistorique(id);
        }
    } while (choix != 0);
}
