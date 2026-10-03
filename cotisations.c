#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cotisations.h"
#include "cycles.h"
#include "fichiers.h"
#include "membres.h"
#include "seances.h"
#include "utils.h"

static int max(int a, int b) { return a > b ? a : b; }
static int somme(int a, int b)
{
    if (b > 0 && a > INT_MAX - b) return INT_MAX;
    if (b < 0 && a < INT_MIN - b) return INT_MIN;
    return a + b;
}

Cotisation *nouvelleCotisation(void) { return calloc(1, sizeof(Cotisation)); }
Dette *nouvelleDette(void) { return calloc(1, sizeof(Dette)); }
Paiement *nouveauPaiement(void) { return calloc(1, sizeof(Paiement)); }

int ajouterCotisationFin(Cycle *c, Cotisation *x)
{
    Cotisation **p;
    if (!c || !x) return 0;
    for (p = &c->cotisations; *p; p = &(*p)->suivant) {}
    x->suivant = NULL;
    *p = x;
    return 1;
}

int ajouterDetteFin(Cycle *c, Dette *x)
{
    Dette **p;
    if (!c || !x) return 0;
    for (p = &c->dettes; *p; p = &(*p)->suivant) {}
    x->suivant = NULL;
    *p = x;
    return 1;
}

int ajouterPaiementFin(Cycle *c, Paiement *x)
{
    Paiement **p;
    if (!c || !x) return 0;
    for (p = &c->paiements; *p; p = &(*p)->suivant) {}
    x->suivant = NULL;
    *p = x;
    return 1;
}

#define LIBERER_LISTE(nom, type) \
void nom(type *p) { while (p) { type *s = p->suivant; free(p); p = s; } }
LIBERER_LISTE(libererCotisations, Cotisation)
LIBERER_LISTE(libererDettes, Dette)
LIBERER_LISTE(libererPaiements, Paiement)

Cotisation *trouverCotisation(const Cycle *c, int seance, int membre)
{
    Cotisation *p;
    if (c) for (p = c->cotisations; p; p = p->suivant)
        if (p->idCycle == c->idCycle && p->numeroSeance == seance &&
            p->idMembre == membre) return p;
    return NULL;
}

Dette *trouverDette(const Cycle *c, int seance, int membre)
{
    Dette *p;
    if (c) for (p = c->dettes; p; p = p->suivant)
        if (p->idCycle == c->idCycle && p->numeroSeanceOrigine == seance &&
            p->idDebiteur == membre) return p;
    return NULL;
}

int penaliteParSeance(const Cycle *c, TypePenalite type)
{
    int taux = type == PENALITE_PARTIELLE ? TAUX_PENALITE_PARTIELLE :
               type == PENALITE_NON_PAIEMENT ? TAUX_PENALITE_NON_PAIEMENT : 0;
    return c && c->montantCotisation > 0 ?
        (int)((long long)c->montantCotisation * taux / 100) : 0;
}

int penaliteRestante(const Dette *d) { return d ? max(0, d->penaliteTotale - d->penalitePayee) : 0; }
int detteRestante(const Dette *d) { return d ? somme(max(0, d->principalRestant), penaliteRestante(d)) : 0; }

DetailDu calculerDetailDu(const Cycle *c, int membre)
{
    DetailDu r = {0, 0, 0, 0};
    Dette *d;
    Seance *s;
    Cotisation *x;
    if (!c) return r;
    for (d = c->dettes; d; d = d->suivant)
        if (d->idCycle == c->idCycle && d->idDebiteur == membre &&
            d->etat != DETTE_REGULARISEE) {
            r.principalDu = somme(r.principalDu, max(0, d->principalRestant));
            r.penalitesDues = somme(r.penalitesDues, penaliteRestante(d));
        }
    s = seanceActuelle(c);
    if (s && s->etat == SEANCE_OUVERTE &&
        (x = trouverCotisation(c, s->numeroSeance, membre)) != NULL &&
        x->etat != COTISATION_PAYEE)
        r.cotisationCouranteDue = max(0, x->montantAttendu - x->montantPaye);
    r.totalDu = somme(somme(r.principalDu, r.penalitesDues),
                      r.cotisationCouranteDue);
    return r;
}

RepartitionPaiement calculerRepartition(const DetailDu *d, int montant)
{
    RepartitionPaiement r = {0, 0, 0};
    int restant = max(0, montant);
    if (!d) return r;
    r.montantDette = max(0, d->principalDu);
    if (r.montantDette > restant) r.montantDette = restant;
    restant -= r.montantDette;
    r.montantPenalite = max(0, d->penalitesDues);
    if (r.montantPenalite > restant) r.montantPenalite = restant;
    restant -= r.montantPenalite;
    r.montantCotisationCourante = max(0, d->cotisationCouranteDue);
    if (r.montantCotisationCourante > restant)
        r.montantCotisationCourante = restant;
    return r;
}

int creerCotisationsSeance(Cycle *c, int numero)
{
    int i;
    if (!c || !c->participants || numero < 1 ||
        numero > c->nombreParticipants || c->montantCotisation < 0) return 0;
    for (i = 0; i < c->nombreParticipants; i++) {
        int id = c->participants[i];
        Cotisation *x = trouverCotisation(c, numero, id);
        if (x) continue;
        x = nouvelleCotisation();
        if (!x) return 0;
        x->idCycle = c->idCycle; x->numeroSeance = numero;
        x->idMembre = id; x->montantAttendu = c->montantCotisation;
        if (!ajouterCotisationFin(c, x)) { free(x); return 0; }
    }
    return 1;
}

static void mettreAJourDette(Dette *d, int seance)
{
    if (d->principalRestant <= 0 && penaliteRestante(d) == 0) {
        d->principalRestant = 0; d->penalitePayee = d->penaliteTotale;
        d->etat = DETTE_REGULARISEE; d->numeroSeanceRegularisation = seance;
    } else if (d->principalRestant < d->principalInitial || d->penalitePayee)
        d->etat = DETTE_PARTIELLEMENT_REGLEE;
}

static void imputerDette(Cycle *c, int membre, int seance, int principal, int penalite)
{
    Dette *d;
    int n;
    for (d = c->dettes; d && principal; d = d->suivant)
        if (d->idDebiteur == membre && d->etat != DETTE_REGULARISEE) {
            n = max(0, d->principalRestant);
            if (n > principal) n = principal;
            d->principalRestant -= n; principal -= n;
            mettreAJourDette(d, seance);
        }
    for (d = c->dettes; d && penalite; d = d->suivant)
        if (d->idDebiteur == membre && d->etat != DETTE_REGULARISEE) {
            n = penaliteRestante(d);
            if (n > penalite) n = penalite;
            d->penalitePayee += n; penalite -= n;
            mettreAJourDette(d, seance);
        }
}

static ResultatPaiement payer(Cycle *c, int membre, int montant,
                              const char *date, Paiement *recu, int cotisationIncluse)
{
    Seance *s;
    Cotisation *x;
    Paiement *p;
    DetailDu d;
    RepartitionPaiement r;
    int total;
    char msg[TAILLE_MESSAGE_HISTORIQUE];
    if (montant <= 0) return PAIEMENT_MONTANT_INVALIDE;
    if (!c || c->etat != CYCLE_EN_COURS) return PAIEMENT_CYCLE_NON_ACTIF;
    if (!membreParticipeAuCycle(c, membre)) return PAIEMENT_MEMBRE_NON_PARTICIPANT;
    s = seanceActuelle(c);
    if (!s || s->etat != SEANCE_OUVERTE) return PAIEMENT_SEANCE_CLOTUREE;
    d = calculerDetailDu(c, membre);
    total = somme(d.principalDu, d.penalitesDues);
    if (cotisationIncluse) total = somme(total, d.cotisationCouranteDue);
    if (total <= 0) return PAIEMENT_RIEN_A_PAYER;
    if (montant > total) return PAIEMENT_SUPERIEUR_AU_DU;
    r = calculerRepartition(&d, montant);
    if (!cotisationIncluse) r.montantCotisationCourante = 0;
    p = nouveauPaiement();
    if (!p) return PAIEMENT_ERREUR_MEMOIRE;
    p->idCycle = c->idCycle; p->numeroSeance = s->numeroSeance;
    p->idMembre = membre; p->montantTotal = montant;
    p->montantDette = r.montantDette; p->montantPenalite = r.montantPenalite;
    p->montantCotisationCourante = r.montantCotisationCourante;
    if (date) { strncpy(p->date, date, TAILLE_DATE - 1); p->date[TAILLE_DATE - 1] = '\0'; }
    if (!ajouterPaiementFin(c, p)) { free(p); return PAIEMENT_ERREUR_MEMOIRE; }
    imputerDette(c, membre, s->numeroSeance, r.montantDette, r.montantPenalite);
    x = trouverCotisation(c, s->numeroSeance, membre);
    if (x && r.montantCotisationCourante) {
        x->montantPaye += r.montantCotisationCourante;
        if (x->montantPaye >= x->montantAttendu) {
            x->montantPaye = x->montantAttendu; x->etat = COTISATION_PAYEE;
        } else x->etat = COTISATION_PARTIELLE;
    }
    c->soldeCaisse = somme(c->soldeCaisse, r.montantPenalite);
    if (recu) { *recu = *p; recu->suivant = NULL; }
    snprintf(msg, sizeof(msg), "Paiement membre %d : %d FCFA (principal %d, penalites %d, cotisation %d)",
             membre, montant, r.montantDette, r.montantPenalite,
             r.montantCotisationCourante);
    ajouterHistorique(c->idCycle, s->numeroSeance, msg);
    return PAIEMENT_OK;
}

ResultatPaiement effectuerPaiement(Cycle *c, int id, int montant,
                                   const char *date, Paiement *recu)
{ return payer(c, id, montant, date, recu, 1); }

ResultatPaiement regulariserDette(Cycle *c, int id, int montant,
                                  const char *date, Paiement *recu)
{ return payer(c, id, montant, date, recu, 0); }

const char *messagePaiement(ResultatPaiement r)
{
    switch (r) {
        case PAIEMENT_OK: return "Le paiement a ete enregistre.";
        case PAIEMENT_MONTANT_INVALIDE: return "Le montant doit etre superieur a zero.";
        case PAIEMENT_SUPERIEUR_AU_DU: return "Paiement refuse : le montant depasse le total du.";
        case PAIEMENT_MEMBRE_NON_PARTICIPANT: return "Ce membre ne participe pas a ce cycle.";
        case PAIEMENT_CYCLE_NON_ACTIF: return "Ce cycle n'accepte pas de paiement.";
        case PAIEMENT_SEANCE_CLOTUREE: return "La seance actuelle est cloturee ou introuvable.";
        case PAIEMENT_RIEN_A_PAYER: return "Ce membre n'a rien a payer pour cette operation.";
        case PAIEMENT_ERREUR_MEMOIRE: return "Erreur : memoire insuffisante.";
    }
    return "Resultat de paiement inconnu.";
}

int creerDettesDepuisSeance(Cycle *c, int numero)
{
    Cotisation *x;
    Seance *s;
    Dette *d;
    int n, creees = 0;
    char msg[TAILLE_MESSAGE_HISTORIQUE];
    if (!c || !(s = trouverSeance(c, numero))) return 0;
    for (x = c->cotisations; x; x = x->suivant) {
        if (x->idCycle != c->idCycle || x->numeroSeance != numero ||
            x->etat == COTISATION_PAYEE || trouverDette(c, numero, x->idMembre))
            continue;
        n = x->montantAttendu - x->montantPaye;
        if (n <= 0) continue;
        d = nouvelleDette();
        if (!d) { fprintf(stderr, "Erreur : memoire insuffisante pour creer une dette.\n"); break; }
        d->idCycle = c->idCycle; d->numeroSeanceOrigine = numero;
        d->idDebiteur = x->idMembre; d->idBeneficiaireOrigine = s->idBeneficiaire;
        d->montantPayeInitial = x->montantPaye;
        d->principalInitial = d->principalRestant = n;
        d->typePenalite = x->montantPaye ? PENALITE_PARTIELLE : PENALITE_NON_PAIEMENT;
        d->etat = DETTE_EN_COURS;
        if (!ajouterDetteFin(c, d)) { free(d); break; }
        snprintf(msg, sizeof(msg), "Dette creee pour le membre %d : principal %d FCFA",
                 d->idDebiteur, n);
        ajouterHistorique(c->idCycle, numero, msg);
        creees++;
    }
    return creees;
}

void appliquerPenalites(Cycle *c, int numero)
{
    Dette *d;
    int retard, ajout;
    long long montant;
    char msg[TAILLE_MESSAGE_HISTORIQUE];
    if (!c) return;
    for (d = c->dettes; d; d = d->suivant) {
        if (d->etat == DETTE_REGULARISEE || d->idCycle != c->idCycle ||
            d->numeroSeanceOrigine > numero) continue;
        retard = numero - d->numeroSeanceOrigine + 1 - d->nombreSeancesRetard;
        if (retard <= 0) continue;
        montant = (long long)penaliteParSeance(c, d->typePenalite) * retard;
        ajout = montant > INT_MAX ? INT_MAX : (int)montant;
        d->nombreSeancesRetard += retard;
        d->penaliteTotale = somme(d->penaliteTotale, ajout);
        if (ajout) {
            snprintf(msg, sizeof(msg), "Penalite appliquee au membre %d : %d FCFA",
                     d->idDebiteur, ajout);
            ajouterHistorique(c->idCycle, numero, msg);
        }
    }
}

void regulariserDettesBeneficiaire(Cycle *c, int id, int numero,
                                   int *principal, int *penalites)
{
    Dette *d;
    char msg[TAILLE_MESSAGE_HISTORIQUE];
    if (principal) *principal = 0;
    if (penalites) *penalites = 0;
    if (!c || !principal || !penalites) return;
    for (d = c->dettes; d; d = d->suivant) {
        int p, f;
        if (d->idCycle != c->idCycle || d->idDebiteur != id ||
            d->etat == DETTE_REGULARISEE) continue;
        p = max(0, d->principalRestant); f = penaliteRestante(d);
        *principal = somme(*principal, p); *penalites = somme(*penalites, f);
        d->principalRestant = 0; d->penalitePayee = d->penaliteTotale;
        d->etat = DETTE_REGULARISEE; d->numeroSeanceRegularisation = numero;
        snprintf(msg, sizeof(msg), "Dette regularisee pour le membre %d : principal %d, penalites %d FCFA",
                 id, p, f);
        ajouterHistorique(c->idCycle, numero, msg);
    }
}

int totalCotisationsAttendues(const Cycle *c)
{
    long long n;
    if (!c || c->nombreParticipants <= 0 || c->montantCotisation <= 0) return 0;
    n = (long long)c->nombreParticipants * c->nombreParticipants * c->montantCotisation;
    return n > INT_MAX ? INT_MAX : (int)n;
}

#define TOTAL(nom, type, liste, champ, filtre) \
int nom(const Cycle *c) { type *p; int n = 0; if (!c) return 0; \
    for (p = c->liste; p; p = p->suivant) if (filtre) n = somme(n, p->champ); \
    return n; }
TOTAL(totalEncaisse, Paiement, paiements, montantTotal, 1)
TOTAL(totalPrincipalRestant, Dette, dettes, principalRestant, 1)
TOTAL(totalPenalitesCalculees, Dette, dettes, penaliteTotale, 1)
TOTAL(totalPenalitesPayees, Dette, dettes, penalitePayee, 1)
int caisseEstCoherente(const Cycle *c) { return c && c->soldeCaisse == totalPenalitesPayees(c); }

void afficherDetailDu(const DetailDu *d)
{
    if (!d) return;
    printf("Principal des dettes : %d FCFA\nPenalites dues : %d FCFA\nCotisation courante : %d FCFA\nTotal : %d FCFA\n",
           d->principalDu, d->penalitesDues, d->cotisationCouranteDue, d->totalDu);
}

void afficherCotisationsSeance(const Tontine *t, const Cycle *c, int numero)
{
    int i, id;
    Cotisation *x;
    if (!c || !trouverSeance(c, numero)) { printf("Numero de seance invalide.\n"); return; }
    printf("\nCotisations - cycle %d, seance %d\n", c->idCycle, numero);
    for (i = 0; i < c->nombreParticipants; i++) {
        id = c->participants[i]; x = trouverCotisation(c, numero, id);
        printf("Membre %d (%s) : %d/%d FCFA - %s\n", id,
               t ? nomDuMembre(&t->membres, id) : "-",
               x ? x->montantPaye : 0, x ? x->montantAttendu : c->montantCotisation,
               x ? libelleEtatCotisation(x->etat) : "NON CREEE");
    }
}

void afficherCotisationsMembre(const Tontine *t, const Cycle *c, int id)
{
    int n;
    Cotisation *x;
    if (!c || !membreParticipeAuCycle(c, id)) { printf("Membre non participant.\n"); return; }
    printf("Cotisations de %s (ID %d)\n", t ? nomDuMembre(&t->membres, id) : "-", id);
    for (n = 1; n <= c->nombreParticipants; n++) {
        x = trouverCotisation(c, n, id);
        printf("Seance %d : %d/%d FCFA - %s\n", n, x ? x->montantPaye : 0,
               x ? x->montantAttendu : c->montantCotisation,
               x ? libelleEtatCotisation(x->etat) : "NON CREEE");
    }
}

void afficherDettes(const Tontine *t, const Cycle *c)
{
    Dette *d;
    int n = 0;
    if (!c) return;
    afficherTitre("DETTES DU CYCLE");
    for (d = c->dettes; d; d = d->suivant) if (d->idCycle == c->idCycle) {
        printf("Seance %d | Debiteur %d | Beneficiaire %s | Principal %d/%d | Retards %d | %s\n",
               d->numeroSeanceOrigine, d->idDebiteur,
               t ? nomDuMembre(&t->membres, d->idBeneficiaireOrigine) : "-",
               d->principalRestant, d->principalInitial, d->nombreSeancesRetard,
               libelleEtatDette(d->etat));
        n++;
    }
    if (!n) puts("Aucune dette.");
}

void afficherPenalites(const Tontine *t, const Cycle *c)
{
    Dette *d;
    int n = 0;
    if (!c) return;
    afficherTitre("PENALITES DU CYCLE");
    for (d = c->dettes; d; d = d->suivant) if (d->idCycle == c->idCycle) {
        printf("Membre %d (%s) | %s : calculees %d, payees %d, restantes %d FCFA\n",
               d->idDebiteur, t ? nomDuMembre(&t->membres, d->idDebiteur) : "-",
               libelleTypePenalite(d->typePenalite), d->penaliteTotale,
               d->penalitePayee, penaliteRestante(d));
        n++;
    }
    if (!n) puts("Aucune penalite.");
}

void afficherPaiements(const Tontine *t, const Cycle *c)
{
    Paiement *p;
    int n = 0;
    if (!c) return;
    for (p = c->paiements; p; p = p->suivant) if (p->idCycle == c->idCycle) {
        printf("Seance %d | Membre %d (%s) | %s | Total %d (principal %d, penalites %d, cotisation %d) FCFA\n",
               p->numeroSeance, p->idMembre,
               t ? nomDuMembre(&t->membres, p->idMembre) : "-",
               p->date, p->montantTotal, p->montantDette, p->montantPenalite,
               p->montantCotisationCourante);
        n++;
    }
    if (!n) puts("Aucun paiement.");
}

static void menuPaiement(Tontine *t, Cycle *c, int detteSeulement)
{
    int id, montant;
    char date[TAILLE_DATE];
    Paiement recu;
    DetailDu d;
    ResultatPaiement r;
    if (!c || c->etat != CYCLE_EN_COURS) { puts(messagePaiement(PAIEMENT_CYCLE_NON_ACTIF)); return; }
    afficherParticipants(t, c);
    id = lireEntier("Identifiant du membre : ", 1, INT_MAX);
    if (!membreParticipeAuCycle(c, id)) { puts(messagePaiement(PAIEMENT_MEMBRE_NON_PARTICIPANT)); return; }
    d = calculerDetailDu(c, id);
    if (detteSeulement) d.cotisationCouranteDue = 0;
    d.totalDu = somme(somme(d.principalDu, d.penalitesDues), d.cotisationCouranteDue);
    afficherDetailDu(&d);
    if (!d.totalDu) { puts(messagePaiement(PAIEMENT_RIEN_A_PAYER)); return; }
    montant = lireEntier("Montant du paiement : ", 1, INT_MAX);
    lireDate("Date (JJ/MM/AAAA) : ", date);
    r = detteSeulement ? regulariserDette(c, id, montant, date, &recu) :
                         effectuerPaiement(c, id, montant, date, &recu);
    puts(messagePaiement(r));
    if (r == PAIEMENT_OK) {
        printf("Recu : %d FCFA (principal %d, penalites %d, cotisation %d), seance %d.\n",
               recu.montantTotal, recu.montantDette, recu.montantPenalite,
               recu.montantCotisationCourante, recu.numeroSeance);
        if (!sauvegarderTout(t)) fprintf(stderr, "Erreur : sauvegarde du paiement impossible.\n");
    }
}

void menuCotiser(Tontine *t, Cycle *c)
{
    if (!t || !c) { puts("Erreur : donnees indisponibles."); return; }
    menuPaiement(t, c, 0);
    attendreEntree();
}

void menuCotisations(Tontine *t)
{
    Cycle *c;
    int choix, sousChoix, id, numero;
    if (!t || !(c = choisirCycle(t, 0))) return;
    do {
        afficherTitre("GESTION DES COTISATIONS");
        printf("Cycle %d\n1. Cotiser\n2. Consulter les cotisations\n3. Dettes\n4. Penalites\n5. Regulariser une dette\n0. Retour\n",
               c->idCycle);
        choix = lireEntier("Votre choix : ", 0, 5);
        switch (choix) {
            case 1: menuCotiser(t, c); break;
            case 2:
                sousChoix = lireEntier("1. Seance  2. Membre : ", 1, 2);
                if (sousChoix == 1) {
                    numero = lireEntier("Numero de seance : ", 1, c->nombreParticipants);
                    afficherCotisationsSeance(t, c, numero);
                } else {
                    afficherParticipants(t, c);
                    id = lireEntier("Identifiant du membre : ", 1, INT_MAX);
                    afficherCotisationsMembre(t, c, id);
                }
                attendreEntree(); break;
            case 3: afficherDettes(t, c); attendreEntree(); break;
            case 4: afficherPenalites(t, c); attendreEntree(); break;
            case 5: menuPaiement(t, c, 1); attendreEntree(); break;
        }
    } while (choix != 0);
}
