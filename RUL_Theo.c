#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "RUL_Theo.h"
#include "Read_Write.h"

/* ======== Outils internes matrices 2x2 / 2x1 ======== */
static inline float f_absf(float x) { return x < 0.0f ? -x : x; }

static void mat2_mul(const float A[4], const float B[4], float C[4]) {
    // C = A*B (2x2)
    C[0] = A[0]*B[0] + A[1]*B[2];
    C[1] = A[0]*B[1] + A[1]*B[3];
    C[2] = A[2]*B[0] + A[3]*B[2];
    C[3] = A[2]*B[1] + A[3]*B[3];
}

static void mat2_vec2(const float A[4], const float x[2], float y[2]) {
    // y = A*x
    y[0] = A[0]*x[0] + A[1]*x[1];
    y[1] = A[2]*x[0] + A[3]*x[1];
}

static void mat2_transpose(const float A[4], float AT[4]) {
    AT[0] = A[0]; AT[1] = A[2];
    AT[2] = A[1]; AT[3] = A[3];
}

/* ======== Interpolation rapide (publique) ======== */
float interp1rapide(const float *X, const float *Y, int n, float x)
{
    if (n <= 0) return 0.0f;
    if (x <= X[0])        return Y[0];
    if (x >= X[n - 1])    return Y[n - 1];

    for (int i = 0; i < n - 1; ++i) {
        if (x >= X[i] && x <= X[i + 1]) {
            float dx = X[i + 1] - X[i];
            if (dx == 0.0f) return Y[i];
            float t = (x - X[i]) / dx;
            return Y[i] + t * (Y[i + 1] - Y[i]);
        }
    }
    return Y[n - 1]; // sécurité
}

/* ======== Estimation RUL (traduction MATLAB) ======== */
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
                    float *RUL_corrige)
{
    // 1) Accumulateur d « équivalent cycles »
    //    MATLAB : integrale_SOC = integrale_SOC + abs(delta_SOC)/dt;
    if (dt > 0.0f) {
        *integrale_SOC += f_absf(delta_SOC) / dt;
    }

    // Condition de déclenchement (quand floor(integrale_SOC/2) augmente)
    int declenche = 0;
    int compteur_attendu = (int)floorf((*integrale_SOC) * 0.5f);
    if (compteur_attendu > *compteur_cycles) {
        *compteur_cycles = *compteur_cycles + 1;
        declenche = 1;
    }

    if (declenche) {
        // 2) Prédiction de l état (RUL, v)
        //    MATLAB :
        //      RUL_pred      = RUL_est - vitesse_degradation*dt;
        //      vitesse_pred  = vitesse_degradation;
        float x_k[2] = { *RUL_est, *vitesse_degradation };
        float x_pred[2];
        mat2_vec2(Fk_RUL, x_k, x_pred);

        // 3) Prédiction de la covariance : Pkkm1 = F*P*F' + Q
        float FP[4], FT[4], Pkkm1[4];
        mat2_mul(Fk_RUL, Pk_RUL, FP);
        mat2_transpose(Fk_RUL, FT);
        mat2_mul(FP, FT, Pkkm1);
        Pkkm1[0] += Qk_RUL[0]; Pkkm1[1] += Qk_RUL[1];
        Pkkm1[2] += Qk_RUL[2]; Pkkm1[3] += Qk_RUL[3];

        // 4) Mesure (loi RUL en fonction du SOH)
        //    MATLAB : z = interp1rapide(X_Loi_RUL, Y_Loi_RUL, SOH)
        float z_mes = interp1rapide(X_Loi_RUL, Y_Loi_RUL, n_loi, SOH);

        // 5) Résidu = z - H*x_pred
        float Hx = Hk_RUL[0]*x_pred[0] + Hk_RUL[1]*x_pred[1];
        float residu = z_mes - Hx;

        // 6) Innovation : Sk = H*Pkkm1*H' + R (scalaire)
        // H (1x2), P (2x2) -> HP (1x2)
        float HP0 = Hk_RUL[0]*Pkkm1[0] + Hk_RUL[1]*Pkkm1[2];
        float HP1 = Hk_RUL[0]*Pkkm1[1] + Hk_RUL[1]*Pkkm1[3];
        float Sk  = HP0*Hk_RUL[0] + HP1*Hk_RUL[1] + Rk_RUL;

        // 7) Gain de Kalman : Kk = Pkkm1*H' / Sk  (2x1)
        float Kk0 = 0.0f, Kk1 = 0.0f;
        if (Sk != 0.0f) {
            // Pkkm1 * H'  = [ P00*H0 + P01*H1,
            //                 P10*H0 + P11*H1 ]^T
            float PHt0 = Pkkm1[0]*Hk_RUL[0] + Pkkm1[1]*Hk_RUL[1];
            float PHt1 = Pkkm1[2]*Hk_RUL[0] + Pkkm1[3]*Hk_RUL[1];
            Kk0 = PHt0 / Sk;
            Kk1 = PHt1 / Sk;
        }

        // 8) Mise à jour état : x = x_pred + Kk * residu
        float x_upd[2];
        x_upd[0] = x_pred[0] + Kk0 * residu;
        x_upd[1] = x_pred[1] + Kk1 * residu;

        // 9) Mise à jour covariance : P = (I - Kk*H) * Pkkm1
        // I - KH =
        // [1 - K0*H0,   -K0*H1
        //  -K1*H0,     1 - K1*H1]
        float IKH[4];
        IKH[0] = 1.0f - Kk0*Hk_RUL[0];
        IKH[1] =      - Kk0*Hk_RUL[1];
        IKH[2] =      - Kk1*Hk_RUL[0];
        IKH[3] = 1.0f - Kk1*Hk_RUL[1];

        mat2_mul(IKH, Pkkm1, Pk_RUL);

        // 10) Renvoyer l état
        *RUL_est = x_upd[0];
        *vitesse_degradation = x_upd[1];
    }

    // 11) Sortie corrigée : RUL_est / vitesse
    if (*vitesse_degradation != 0.0f) {
        *RUL_corrige = (*RUL_est) / (*vitesse_degradation);
    } else {
        *RUL_corrige = 0.0f; // sécurité anti-division par 0
    }
}

/* ======== Setup d'exemple (aligné avec *_Theo.c) ======== */
void setup(void)
{
    // === Entrées (buffers fournis par Charge_donnees) ===
    const float *courant     = NULL;
    const float *tension     = NULL;
    const float *temperature = NULL;
    const float *SOH_buf     = NULL;
    const float *SOC_buf     = NULL;

    // Charge des données (adapter nb_samples selon la source)
    size_t nb_samples = 1000000;
    Charge_donnees(&courant, &tension, &temperature, &SOH_buf, &SOC_buf);

    // === Paramètres Kalman (exemple cohérent avec le modèle MATLAB)
    // Modèle constant-velocity discret : x=[RUL; v], F=[1 -dt; 0 1], H=[1 0]
    float dt = 1.0f;

    // Matrices (seront reconstruites à chaque pas si dt varie)
    float F[4]  = { 1.0f, -dt, 0.0f, 1.0f };
    float H[2]  = { 1.0f,  0.0f };
    float Q[4]  = { 1e-4f, 0.0f, 0.0f, 1e-6f };
    float R     = 1e-3f;
    float P[4]  = { 1.0f, 0.0f, 0.0f, 1.0f };

    // === Loi RUL(SOH) — EXEMPLE (remplacer par vos vraies tables)
    static const float X_Loi_RUL[5] = { 0.70f, 0.80f, 0.90f, 0.95f, 1.00f };
    static const float Y_Loi_RUL[5] = { 200.0f, 400.0f, 800.0f, 1200.0f, 2000.0f };
    const int n_loi = 5;

    // === États internes
    float RUL_est = Y_Loi_RUL[n_loi - 1];   // init plausible
    float v_deg   = 1.0f;                   // vitesse initiale
    float integSOC = 0.0f;
    int   cycles   = 0;
    float RUL_corr = 0.0f;

    // === Boucle de traitement
    for (size_t i = 0; i < nb_samples; ++i) {
        float SOH = SOH_buf[i];
        // delta_SOC : SOC(i) - SOC(i-1)
        float delta_SOC = 0.0f;
        if (i > 0) delta_SOC = SOC_buf[i] - SOC_buf[i - 1];

        // Recalculer F si dt varie :
        F[0] = 1.0f; F[1] = -dt; F[2] = 0.0f; F[3] = 1.0f;

        estimation_RUL(SOH,
                       delta_SOC,
                       dt,
                       X_Loi_RUL,
                       Y_Loi_RUL,
                       n_loi,
                       F, H, Q, R,
                       &RUL_est, &v_deg,
                       &integSOC, &cycles,
                       P,
                       &RUL_corr);

        // Exemple d’affichage
        // printf("i=%zu SOH=%.4f RUL=%.3f v=%.6f RUL_corr=%.3f\n",
        //        i, SOH, RUL_est, v_deg, RUL_corr);
    }

    Free_donnees(courant, tension, temperature, SOH_buf, SOC_buf);
}

int main(void) {
    setup();
    printf("Fin du programme\n");
    return 0;
}

