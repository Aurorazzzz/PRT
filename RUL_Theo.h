#ifndef RUL_THEO_H
#define RUL_THEO_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Interpolation linéaire 1D : équivalent de interp1rapide(X,Y,x) */
float interp1rapide(const float *X, const float *Y, int n, float x);

/* Estimation RUL (mise à jour in-place via pointeurs) */
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

/* Exemple de point d’entrée : charge, boucle, écrit vecteur_RUL */
void setup(void);

#ifdef __cplusplus
}
#endif

#endif /* RUL_THEO_H */
