#ifndef RUL_THEO_H
#define RUL_THEO_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Interpolation linéaire rapide 1D, équivalent de interp1rapide(X, Y, x).
 *
 * @param X  Tableau d'abscisses (croissant), taille n
 * @param Y  Tableau d'ordonnées correspondant, taille n
 * @param n  Taille des tableaux
 * @param x  Abscisse d'interpolation
 * @return   Valeur interpolée
 */
float interp1rapide(const float *X, const float *Y, int n, float x);

/**
 * Estimation RUL (traduction C du MATLAB).
 * État : x = [RUL; vitesse_degradation].
 * Modèle par défaut (si tu choisis F=[1 -dt; 0 1], H=[1 0]):
 *   x_pred = F * x      avec    RUL_pred = RUL - v*dt,   v_pred = v
 *   z      = RUL_modele(SOH)    (mesure issue de la loi RUL(SOH))
 *
 * @param SOH                   État de santé courant [0..1]
 * @param delta_SOC             Incrément de SOC depuis l’instant k-1 (peut être ±)
 * @param dt                    Pas de temps (s)
 * @param X_Loi_RUL, Y_Loi_RUL  Loi RUL(SOH) (abscisses = SOH, ordonnées = RUL)
 * @param n_loi                 Taille des tables de loi
 * @param Fk_RUL (2x2), Hk_RUL (1x2), Qk_RUL (2x2), Rk_RUL (scalaire)  Paramètres Kalman
 * @param RUL_est [in/out]      Composante RUL de l’état
 * @param vitesse_degradation [in/out]  Composante vitesse de l’état
 * @param integrale_SOC [in/out] Accumulateur d’|delta_SOC|/dt (≈ demi-cycles)
 * @param compteur_cycles [in/out] Compteur de cycles (entier)
 * @param Pk_RUL (2x2) [in/out] Matrice de covariance
 * @param RUL_corrige [out]      RUL_est / vitesse_degradation (sécurisé contre /0)
 */
void estimation_RUL(float SOH,
                    float delta_SOC,
                    float dt,
                    const float *X_Loi_RUL,
                    const float *Y_Loi_RUL,
                    int n_loi,
                    const float Fk_RUL[4],
                    const float Hk_RUL[2],
                    const float Qk_RUL[4],
                    float Rk_RUL,
                    float *RUL_est,
                    float *vitesse_degradation,
                    float *integrale_SOC,
                    int   *compteur_cycles,
                    float Pk_RUL[4],
                    float *RUL_corrige);

/**
 * Exemple de point d’entrée (chargement des buffers + boucle),
 * comme dans tes modules *_Theo.c.
 */
void setup(void);

#ifdef __cplusplus
}
#endif

#endif /* RUL_THEO_H */
