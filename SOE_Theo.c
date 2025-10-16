/*
 * estimation_SOE.c
 * Version: C99, autonome, sans dépendances.
 *
 * Usage 1 (scalaires sans fichier):
 *   ./estimation_SOE --scalars SOC SOH moins_eta_sur_Q x_ocv ocv_int
 *   Exemple:
 *   ./estimation_SOE --scalars 0.55 0.98 -0.002 0.55 12.34
 *   (dans ce mode, l'interpolation est triviale car x_ocv == SOC; ocv_int est fourni directement)
 *
 * Usage 2 (avec CSV tabulé pour l'interpolation):
 *   ./estimation_SOE --csv chemin.csv SOC SOH moins_eta_sur_Q
 *   - chemin.csv: 2 colonnes séparées par des virgules: X_OCV,LOI_INTEG_OCV_DECHARGE
 *   - SOC sera interpolé linéairement dans cette table.
 *   Exemple:
 *   ./estimation_SOE --csv ocv_table.csv 0.55 0.98 -0.002
 *
 * Sortie:
 *   Ecrit le SOE_pred sur stdout avec 10 décimales.
 *
 * Notes embarquées:
 *   - Pas d'allocation dynamique si on fixe MAX_SAMPLES.
 *   - Possibilité de remplacer l'I/O par des interfaces matériels au besoin.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "SOE_Theo.h"


#ifndef MAX_SAMPLES
#define MAX_SAMPLES 1024
#endif

/* Interpolation linéaire: renvoie y tel que (x, y) est interpolé
 * dans (xp[0..n-1], fp[0..n-1]).
 * Hypothèses:
 *   - n >= 2
 *   - xp strictement croissant (monotone).
 * Comportement hors-bande:
 *   - Clamp (saturation) aux extrémités.
 */
static double interp1_linear(const double *xp, const double *fp, size_t n, double x)
{
    if (n == 0) return 0.0;
    if (n == 1) return fp[0];

    // Saturation aux bornes
    if (x <= xp[0])  return fp[0];
    if (x >= xp[n-1]) return fp[n-1];

    // Recherche linéaire (rapide à implémenter; en embarqué, on peut passer à une bisection)
    // Complexité O(n). Pour des tables grandes, remplacer par une recherche dichotomique.
    for (size_t i = 0; i < n - 1; ++i) {
        double x0 = xp[i];
        double x1 = xp[i+1];
        if (x >= x0 && x <= x1) {
            double y0 = fp[i];
            double y1 = fp[i+1];

            double dx = x1 - x0;
            if (dx == 0.0) {
                // Sécurité: évite division par zéro si table corrompue
                return y0;
            }
            double t = (x - x0) / dx;
            return y0 + t * (y1 - y0);
        }
    }

    // Ne devrait pas arriver si les conditions sont respectées
    return fp[n-1];
}

/* Calcul de l'intégrale de courant prédite (terme multiplicatif)
 * integrale_courant_pred = (1 / moins_eta_sur_Q) * SOH * SOC
 */
static double integrale_courant_pred(double SOC, double SOH, double moins_eta_sur_Q)
{
    // Sécurité: éviter division par zéro
    if (moins_eta_sur_Q == 0.0) {
        // En embarqué, on pourrait renvoyer une valeur sentinelle ou lever un flag d'erreur.
        // Ici, on sature à 0 pour éviter NaN/Inf.
        return 0.0;
    }
    return (1.0 / moins_eta_sur_Q) * SOH * SOC;
}

/* estimation_SOE: version avec interpolation tabulée */
static double estimation_SOE_from_table(double SOC, double SOH, double moins_eta_sur_Q,
                                        const double *X_OCV, const double *LOI_INTEG_OCV_DECHARGE,
                                        size_t n)
{
    double ocv_int = interp1_linear(X_OCV, LOI_INTEG_OCV_DECHARGE, n, SOC);
    double Iint = integrale_courant_pred(SOC, SOH, moins_eta_sur_Q);
    return ocv_int * Iint;
}

/* estimation_SOE: version scalaire (si ocv_int à SOC est déjà connu) */
static double estimation_SOE_scalar(double SOC, double SOH, double moins_eta_sur_Q, double ocv_int_at_SOC)
{
    double Iint = integrale_courant_pred(SOC, SOH, moins_eta_sur_Q);
    return ocv_int_at_SOC * Iint;
}

/* Lecture d’un CSV simple "X_OCV,LOI_INTEG_OCV_DECHARGE" (en-tête optionnel).
 * Remplit xp[], fp[] et renvoie le nombre d’échantillons lus.
 * Retourne 0 en cas d’erreur.
 */
static size_t read_csv_table(const char *path, double *xp, double *fp, size_t maxn)
{
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    char line[256];
    size_t count = 0;
    while (fgets(line, sizeof(line), f) && count < maxn) {
        // Ignore les lignes vides
        if (line[0] == '\n' || line[0] == '\r') continue;

        // Tente de parser deux doubles séparés par virgule
        // On tolère un en-tête: si sscanf échoue, on saute la ligne.
        double x, y;
        // Autoriser virgule ou point-virgule
        char *p = strchr(line, ';');
        if (p) *p = ',';

        int nread = sscanf(line, " %lf , %lf", &x, &y);
        if (nread == 2) {
            xp[count] = x;
            fp[count] = y;
            count++;
        } else {
            // Ligne non conforme -> on l’ignore (en-tête probable)
            continue;
        }
    }
    fclose(f);

    return count;
}

/* Vérifie que le vecteur est strictement croissant (condition pour interpolation) */
static bool is_strictly_increasing(const double *x, size_t n)
{
    if (n < 2) return false;
    for (size_t i = 1; i < n; ++i) {
        if (!(x[i] > x[i-1])) return false;
    }
    return true;
}

static void print_usage(const char *prog)
{
    fprintf(stderr,
        "Usage:\n"
        "  %s --scalars SOC SOH moins_eta_sur_Q x_ocv ocv_int\n"
        "    (cas trivial: ocv_int fourni pour SOC=x_ocv)\n"
        "  %s --csv path.csv SOC SOH moins_eta_sur_Q\n"
        "    (interpolation depuis la table X_OCV,LOI_INTEG_OCV_DECHARGE)\n",
        prog, prog);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--scalars") == 0) {
        if (argc != 7) {
            print_usage(argv[0]);
            return 1;
        }
        double SOC              = atof(argv[2]);
        double SOH              = atof(argv[3]);
        double moins_eta_sur_Q  = atof(argv[4]);
        double x_ocv            = atof(argv[5]);  // non utilisé sauf pour cohérence/trace
        double ocv_int_at_SOC   = atof(argv[6]);

        (void)x_ocv; // évite un warning si non utilisé

        double SOE_pred = estimation_SOE_scalar(SOC, SOH, moins_eta_sur_Q, ocv_int_at_SOC);
        printf("%.10f\n", SOE_pred);
        return 0;
    }
    else if (strcmp(argv[1], "--csv") == 0) {
        if (argc != 6) {
            print_usage(argv[0]);
            return 1;
        }
        const char *csv_path    = argv[2];
        double SOC              = atof(argv[3]);
        double SOH              = atof(argv[4]);
        double moins_eta_sur_Q  = atof(argv[5]);

        double X_OCV[MAX_SAMPLES];
        double LOI_INT[MAX_SAMPLES];

        size_t n = read_csv_table(csv_path, X_OCV, LOI_INT, MAX_SAMPLES);
        if (n < 2) {
            fprintf(stderr, "Erreur: table CSV invalide ou insuffisante (n=%zu).\n", n);
            return 2;
        }
        if (!is_strictly_increasing(X_OCV, n)) {
            fprintf(stderr, "Erreur: X_OCV doit être strictement croissant.\n");
            return 3;
        }

        double SOE_pred = estimation_SOE_from_table(SOC, SOH, moins_eta_sur_Q, X_OCV, LOI_INT, n);
        printf("%.10f\n", SOE_pred);
        return 0;
    }
    else {
        print_usage(argv[0]);
        return 1;
    }
}
