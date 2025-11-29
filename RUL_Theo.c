#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "RUL_Theo.h"
#include "Read_Write.h"

/* ================== Helpers matrices 2x2 / 2x1 ================== */
static inline float f_absf(float x) { return x < 0.0f ? -x : x; }

static inline void mat2_mul(const float A[4], const float B[4], float C[4]) {
    C[0] = A[0]*B[0] + A[1]*B[2];
    C[1] = A[0]*B[1] + A[1]*B[3];
    C[2] = A[2]*B[0] + A[3]*B[2];
    C[3] = A[2]*B[1] + A[3]*B[3];
}

static inline void mat2_vec2(const float A[4], const float x[2], float y[2]) {
    y[0] = A[0]*x[0] + A[1]*x[1];
    y[1] = A[2]*x[0] + A[3]*x[1];
}

static inline void mat2_transpose(const float A[4], float AT[4]) {
    AT[0] = A[0]; AT[1] = A[2];
    AT[2] = A[1]; AT[3] = A[3];
}

/* ================== Estimation RUL (Kalman 2x2) ================== */
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
    /* 1) Accumulateur de demi-cycles (selon ton MATLAB) */
    if (dt > 0.0f) *integrale_SOC += f_absf(delta_SOC) / dt;
    //printf("%f\n", *integrale_SOC);

    /* 2) Déclenchement quand floor(integrale_SOC/2) augmente */
    int declenche = 0;
    int compteur_attendu = (int)floorf((*integrale_SOC) * 0.5f);
    if (compteur_attendu > *compteur_cycles) {
        *compteur_cycles += 1;
        declenche = 1;
    }

    if (declenche) {
        // Creation du vecteur Xk avec l'etat present
        float x_k[2] = { *RUL_est, *vitesse_degradation };
        float x_pred[2];
        // calcul de l'etat predit (k|k-1)
        mat2_vec2(Fk_RUL, x_k, x_pred);

        /* Pkkm1 = F P F' + Q */
        // Calcul de Pkkm1
        float FP[4], FT[4], Pkkm1[4];
        mat2_mul(Fk_RUL, Pk_RUL, FP);
        mat2_transpose(Fk_RUL, FT);
        mat2_mul(FP, FT, Pkkm1);
        //Ajout de Qk
        Pkkm1[0] += Qk_RUL[0]; Pkkm1[1] += Qk_RUL[1];
        Pkkm1[2] += Qk_RUL[2]; Pkkm1[3] += Qk_RUL[3];

        /* Mesure z = RUL_modele(SOH) via loi RUL(SOH) */
        // On trouve RUL_est
        float z_mes = interp1Drapide(X_Loi_RUL, Y_Loi_RUL, n_loi, SOH);

        /* Résidu */
        //float Hx = Hk_RUL[0]*x_pred[0] + Hk_RUL[1]*x_pred[1];
        float residu = z_mes - x_pred[0];

        /* Innovation Sk = H Pkkm1 H' + R */
        float HP0 = Hk_RUL[0]*Pkkm1[0] + Hk_RUL[1]*Pkkm1[2];
        float HP1 = Hk_RUL[0]*Pkkm1[1] + Hk_RUL[1]*Pkkm1[3];
        float Sk  = HP0*Hk_RUL[0] + HP1*Hk_RUL[1] + Rk_RUL;

        /* Gain de Kalman Kk */
        float Kk0 = 0.0f, Kk1 = 0.0f;
        if (Sk != 0.0f) {
            float PHt0 = Pkkm1[0]*Hk_RUL[0] + Pkkm1[1]*Hk_RUL[1];
            float PHt1 = Pkkm1[2]*Hk_RUL[0] + Pkkm1[3]*Hk_RUL[1];
            Kk0 = PHt0 / Sk;
            Kk1 = PHt1 / Sk;
        }

        /* Mise à jour état */
        float x_upd[2];
        x_upd[0] = x_pred[0] + Kk0 * residu;
        x_upd[1] = x_pred[1] + Kk1 * residu;

        /* Mise à jour covariance : (I - K H) Pkkm1 */
        float IKH[4];
        IKH[0] = 1.0f - Kk0*Hk_RUL[0];
        IKH[1] =      - Kk0*Hk_RUL[1];
        IKH[2] =      - Kk1*Hk_RUL[0];
        IKH[3] = 1.0f - Kk1*Hk_RUL[1];
        mat2_mul(IKH, Pkkm1, Pk_RUL);

        *RUL_est = x_upd[0];
        *vitesse_degradation = x_upd[1];
    }

    /* Sortie corrigée : RUL / vitesse (protégée) */
    *RUL_corrige = (*vitesse_degradation != 0.0f)
                 ? (*RUL_est) / (*vitesse_degradation)
                 : 0.0f;
    
    //printf("%f\n", *RUL_est);
    //printf("%f\n", *RUL_corrige);

}

/* ================== Setup d'exemple ================== */
void RUL_setup(void)
{
    /* Entrées (buffers fournis par Read_Write.h) */
    const float *courant = NULL, *tension = NULL, *temperature = NULL;
    const float *SOH_vec = NULL, *SOC = NULL;

    /* Adapter cette valeur à la taille réelle de tes buffers */
    const size_t NbIteration = 1000000;

    float *vecteur_RUL = (float*)malloc(NbIteration * sizeof(float));
    if (!vecteur_RUL) { perror("malloc vecteur_RUL"); return; }

    /* Charge les données */
    Charge_donnees(&courant, &tension, &temperature, &SOH_vec, &SOC);

    /* Paramètres Kalman */
    float dt = 1.0f;
    float F[4]  = { 1.0f, -1, 0.0f, 1.0f };
    float H[2]  = { 1.0f,  0.0f };
    float Q[4]  = { 6.8870745, 0.0f, 0.0f, 1.0017803e-5f };
    float R     = 9.9969953e+4f;
    float P[4]  = { 989.54010f, -1.4191452f, -1.4191453f, 0.0129244f };

    /* Loi RUL(SOH) */
    static const float X_Loi_RUL[9] = { 0.7146760f, 0.7777946f, 0.8302932f, 0.8483074f, 0.8658035f, 0.8909407f, 0.9458607f, 0.9985912f, 1.0f };
    static const float Y_Loi_RUL[9] = { -3.5574283e2f, -1.0574284e2f, 144.25717f, 244.25717f, 394.25717f, 644.25714, 1144.2572, 1594.2572, 1644.2572 };
    const int n_loi = 9;

    /* États internes */
    float RUL_est = interp1Drapide(X_Loi_RUL, Y_Loi_RUL, n_loi, 1.0);
    float v_deg   = 1.0f;
    float integSOC = 0.0f;
    int   cycles   = 0;
    float RUL_corr = 0.0f;

    /* Boucle principale : on remplit vecteur_RUL avec le RUL corrigé */
    for (size_t i = 0; i < NbIteration; ++i) {
        float SOH = SOH_vec[i];
        float delta_SOC = (i > 0) ? (SOC[i] - SOC[i - 1]) : 0.0f;

        /* Recalc F si dt varie */
        F[0] = 1.0f; F[1] = -dt; F[2] = 0.0f; F[3] = 1.0f;

        estimation_RUL(SOH, delta_SOC, dt,
                       X_Loi_RUL, Y_Loi_RUL, n_loi,
                       F, H, Q, R,
                       &RUL_est, &v_deg,
                       &integSOC, &cycles,
                       P,
                       &RUL_corr);

        vecteur_RUL[i] = RUL_corr; 
    }

    /* Écriture du résultat et nettoyage */
    Ecriture_result(vecteur_RUL, (int)NbIteration, "RUL_raspi_result");
    Free_donnees(courant, tension, temperature, SOH_vec, SOC);
}


/*int main(void) {
    setup();
    puts("Fin du programme");
    return 0;
}*/

