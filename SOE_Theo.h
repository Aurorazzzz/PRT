/*
 * estimation_SOE.h
 * Interface pour l'estimation du State of Energy (SOE)
 * Version : C99 compatible, embarquable
 *
 */

#ifndef ESTIMATION_SOE_H
#define ESTIMATION_SOE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>  /* pour size_t */

/* -------------------------------------------------------------------------- */
/* Constantes configurables (optionnelles)                                    */
/* -------------------------------------------------------------------------- */

/* Nombre maximum d’échantillons pour la table OCV.
 * Peut être redéfini avant l’inclusion de ce header si besoin :
 *   #define MAX_SAMPLES 512
 */
#ifndef MAX_SAMPLES
#define MAX_SAMPLES 1024
#endif

/* -------------------------------------------------------------------------- */
/* Fonctions principales                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief Interpolation linéaire 1D (équivalent MATLAB interp1).
 *
 * @param xp   Tableau des abscisses (X_OCV), strictement croissant.
 * @param fp   Tableau des ordonnées (LOI_INTEG_OCV_DECHARGE).
 * @param n    Nombre d’échantillons.
 * @param x    Point où interpoler.
 * @return     Valeur interpolée, saturée aux bornes si x < xp[0] ou x > xp[n-1].
 */
double interp1_linear(const double *xp, const double *fp, size_t n, double x);

/**
 * @brief Calcule le terme d’intégrale de courant prédite :
 *        integrale_courant_pred = (1 / moins_eta_sur_Q) * SOH * SOC
 *
 * @param SOC              State of Charge (0 à 1)
 * @param SOH              State of Health (0 à 1)
 * @param moins_eta_sur_Q  Terme équivalent à (-eta / Q)
 * @return                 Valeur du terme d’intégration
 */
double integrale_courant_pred(double SOC, double SOH, double moins_eta_sur_Q);

/**
 * @brief Estimation complète du SOE avec table d’OCV intégrée.
 *
 * @param SOC                       State of Charge
 * @param SOH                       State of Health
 * @param moins_eta_sur_Q            Terme (-eta / Q)
 * @param X_OCV                     Tableau des SOC (abscisses)
 * @param LOI_INTEG_OCV_DECHARGE    Tableau des valeurs intégrées d’OCV
 * @param n                         Taille des tableaux
 * @return                          SOE prédit (SOE_pred)
 */
double estimation_SOE_from_table(double SOC, double SOH, double moins_eta_sur_Q,
                                 const double *X_OCV,
                                 const double *LOI_INTEG_OCV_DECHARGE,
                                 size_t n);

/**
 * @brief Variante simplifiée si la valeur interpolée d’OCV (ocv_int_at_SOC)
 *        est déjà connue pour le SOC courant.
 *
 * @param SOC             State of Charge
 * @param SOH             State of Health
 * @param moins_eta_sur_Q (-eta / Q)
 * @param ocv_int_at_SOC  Valeur intégrée d’OCV correspondante à SOC
 * @return                SOE prédit
 */
double estimation_SOE_scalar(double SOC, double SOH,
                             double moins_eta_sur_Q, double ocv_int_at_SOC);

/* -------------------------------------------------------------------------- */
/* Fonctions utilitaires facultatives                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Lit un fichier CSV contenant la table OCV (X_OCV, LOI_INTEG_OCV_DECHARGE).
 *        Format attendu : deux colonnes séparées par une virgule.
 *
 * @param path   Chemin du fichier CSV
 * @param xp     Tableau de sortie (X_OCV)
 * @param fp     Tableau de sortie (LOI_INTEG_OCV_DECHARGE)
 * @param maxn   Taille maximale allouée pour xp/fp
 * @return       Nombre de lignes valides lues (0 en cas d’erreur)
 */
size_t read_csv_table(const char *path, double *xp, double *fp, size_t maxn);

/**
 * @brief Vérifie que le vecteur est strictement croissant.
 *
 * @param x   Tableau à tester
 * @param n   Taille du tableau
 * @return    true si croissant, false sinon
 */
int is_strictly_increasing(const double *x, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* ESTIMATION_SOE_H */
