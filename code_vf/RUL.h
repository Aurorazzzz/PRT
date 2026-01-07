#ifndef RUL_H
#define RUL_H

// ============================================================================
// Contexte RUL : paramètres + états internes du filtre de Kalman
// ============================================================================

typedef struct
{
    // Paramètres de dynamique
    float dt;       // pas de temps

    // Matrices du modèle de Kalman (2x2 ou 1x2)
    float F[4];     // matrice d'état 2x2
    float H[2];     // matrice d'observation 1x2
    float Q[4];     // covariance de bruit de processus 2x2
    float R;        // variance de bruit de mesure
    float P[4];     // covariance d'état 2x2

    // Loi RUL(SOH)
    const float *X_Loi_RUL;
    const float *Y_Loi_RUL;
    int          n_loi;

    // États internes du filtre
    float RUL_est;             // composante d'état : RUL
    float vitesse_degradation; // composante d'état : vitesse de dégradation

    float integrale_SOC;       // accumulateur de |ΔSOC|/dt
    int   compteur_cycles;     // compteur de demi-cycles
    float SOC_precedent;       // pour calculer ΔSOC
    int   first_call;          // pour gérer le premier échantillon

} RUL_Context;

// ============================================================================
// Initialisation du contexte RUL
// ============================================================================
void RUL_init(RUL_Context *ctx);

// ============================================================================
// Step RUL : un échantillon (SOH, SOC) → mise à jour + RUL corrigé
//
// Entrées :
//   - ctx : contexte déjà initialisé
//   - SOH : état de santé (0–1)
//   - SOC : état de charge (0–1)
//
// Sortie :
//   - valeur RUL corrigée = RUL_est / vitesse_degradation (protégée)
//
// Le contexte interne (matrices, covariances, RUL_est, etc.) est mis à jour.
// ============================================================================
float RUL_step(RUL_Context *ctx, float SOH, float SOC);

#endif // RUL_THEO_H
