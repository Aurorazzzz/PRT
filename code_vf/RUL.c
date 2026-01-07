#include <math.h>
#include "RUL.h"

// Prototype de l'interpolation (implémentée ailleurs)
float interp1Drapide(const float *x, const float *y, int n, float x_req);

/* ================== Helpers matrices 2x2 / 2x1 ================== */

static inline float f_absf(float x) { return x < 0.0f ? -x : x; }

static inline void mat2_mul(const float A[4], const float B[4], float C[4])
{
    C[0] = A[0]*B[0] + A[1]*B[2];
    C[1] = A[0]*B[1] + A[1]*B[3];
    C[2] = A[2]*B[0] + A[3]*B[2];
    C[3] = A[2]*B[1] + A[3]*B[3];
}

static inline void mat2_vec2(const float A[4], const float x[2], float y[2])
{
    y[0] = A[0]*x[0] + A[1]*x[1];
    y[1] = A[2]*x[0] + A[3]*x[1];
}

static inline void mat2_transpose(const float A[4], float AT[4])
{
    AT[0] = A[0]; AT[1] = A[2];
    AT[2] = A[1]; AT[3] = A[3];
}

/* ================== Loi RUL(SOH) ================== */

static const float X_Loi_RUL_tab[9] = {
    0.7146760f, 0.7777946f, 0.8302932f, 0.8483074f,
    0.8658035f, 0.8909407f, 0.9458607f, 0.9985912f, 1.0f
};

static const float Y_Loi_RUL_tab[9] = {
   -3.5574283e2f, -1.0574284e2f, 144.25717f, 244.25717f,
    394.25717f,   644.25714f,   1144.2572f, 1594.2572f, 1644.2572f
};

static const int N_LOI_RUL = 9;

/* ================== Noyau Kalman : estimation RUL sur un pas ================== */

static void estimation_RUL_core(RUL_Context *ctx, float SOH, float delta_SOC)
{
    if (!ctx) return;

    /* 1) Accumulateur de demi-cycles (comme dans votre version) */
    if (ctx->dt > 0.0f) {
        ctx->integrale_SOC += f_absf(delta_SOC) / ctx->dt;
    }

    /* 2) Déclenchement quand floor(integrale_SOC/2) augmente */
    int declenche = 0;
    int compteur_attendu = (int)floorf(ctx->integrale_SOC * 0.5f);
    if (compteur_attendu > ctx->compteur_cycles) {
        ctx->compteur_cycles += 1;
        declenche = 1;
    }

    if (declenche) {
        /* Mise à jour de F si besoin (dt peut évoluer) */
        ctx->F[0] = 1.0f;
        ctx->F[1] = -ctx->dt;
        ctx->F[2] = 0.0f;
        ctx->F[3] = 1.0f;

        /* Vecteur d'état courant x_k = [RUL_est ; vitesse_degradation] */
        float x_k[2]   = { ctx->RUL_est, ctx->vitesse_degradation };
        float x_pred[2];

        /* x_pred = F x_k (prédiction) */
        mat2_vec2(ctx->F, x_k, x_pred);

        /* Pkkm1 = F P F' + Q */
        float FP[4], FT[4], Pkkm1[4];
        mat2_mul(ctx->F, ctx->P, FP);
        mat2_transpose(ctx->F, FT);
        mat2_mul(FP, FT, Pkkm1);

        /* Ajout de Q */
        Pkkm1[0] += ctx->Q[0]; Pkkm1[1] += ctx->Q[1];
        Pkkm1[2] += ctx->Q[2]; Pkkm1[3] += ctx->Q[3];

        /* Mesure z = RUL_modele(SOH) via loi RUL(SOH) */
        float z_mes = interp1Drapide(ctx->X_Loi_RUL,
                                     ctx->Y_Loi_RUL,
                                     ctx->n_loi,
                                     SOH);

        /* Résidu : z - H x_pred ; ici H = [1 0], donc H*x_pred = x_pred[0] */
        float residu = z_mes - x_pred[0];

        /* Innovation Sk = H Pkkm1 H' + R */
        float HP0 = ctx->H[0]*Pkkm1[0] + ctx->H[1]*Pkkm1[2];
        float HP1 = ctx->H[0]*Pkkm1[1] + ctx->H[1]*Pkkm1[3];
        float Sk  = HP0*ctx->H[0] + HP1*ctx->H[1] + ctx->R;

        /* Gain de Kalman Kk */
        float Kk0 = 0.0f, Kk1 = 0.0f;
        if (Sk != 0.0f) {
            float PHt0 = Pkkm1[0]*ctx->H[0] + Pkkm1[1]*ctx->H[1];
            float PHt1 = Pkkm1[2]*ctx->H[0] + Pkkm1[3]*ctx->H[1];
            Kk0 = PHt0 / Sk;
            Kk1 = PHt1 / Sk;
        }

        /* Mise à jour état x_upd = x_pred + K * residu */
        float x_upd[2];
        x_upd[0] = x_pred[0] + Kk0 * residu;
        x_upd[1] = x_pred[1] + Kk1 * residu;

        /* Mise à jour covariance : (I - K H) Pkkm1 */
        float IKH[4];
        IKH[0] = 1.0f - Kk0*ctx->H[0];
        IKH[1] =      - Kk0*ctx->H[1];
        IKH[2] =      - Kk1*ctx->H[0];
        IKH[3] = 1.0f - Kk1*ctx->H[1];
        mat2_mul(IKH, Pkkm1, ctx->P);

        ctx->RUL_est            = x_upd[0];
        ctx->vitesse_degradation = x_upd[1];
    }

    /* Sinon, pas de déclenchement : on garde RUL_est / vitesse_degradation */
}

/* ================== Initialisation du contexte ================== */

void RUL_init(RUL_Context *ctx)
{
    if (!ctx) return;

    ctx->dt = 1.0f;

    /* Matrices (mêmes valeurs que dans votre RUL_setup) */
    ctx->F[0] = 1.0f; ctx->F[1] = -ctx->dt;
    ctx->F[2] = 0.0f; ctx->F[3] = 1.0f;

    ctx->H[0] = 1.0f;
    ctx->H[1] = 0.0f;

    ctx->Q[0] = 6.8870745f;    ctx->Q[1] = 0.0f;
    ctx->Q[2] = 0.0f;          ctx->Q[3] = 1.0017803e-5f;

    ctx->R    = 9.9969953e4f;

    ctx->P[0] = 989.54010f;
    ctx->P[1] = -1.4191452f;
    ctx->P[2] = -1.4191453f;
    ctx->P[3] = 0.0129244f;

    /* Loi RUL(SOH) */
    ctx->X_Loi_RUL = X_Loi_RUL_tab;
    ctx->Y_Loi_RUL = Y_Loi_RUL_tab;
    ctx->n_loi     = N_LOI_RUL;

    /* Initialisation des états internes */
    ctx->RUL_est = interp1Drapide(ctx->X_Loi_RUL,
                                  ctx->Y_Loi_RUL,
                                  ctx->n_loi,
                                  1.0f);      // SOH=1

    ctx->vitesse_degradation = 1.0f;
    ctx->integrale_SOC       = 0.0f;
    ctx->compteur_cycles     = 0;
    ctx->SOC_precedent       = 0.0f;
    ctx->first_call          = 1;
}

/* ================== Step : un échantillon SOH, SOC ================== */

float RUL_step(RUL_Context *ctx, float SOH, float SOC)
{
    if (!ctx) return 0.0f;

    /* Calcul de delta_SOC (0 au premier appel) */
    float delta_SOC = 0.0f;
    if (ctx->first_call) {
        ctx->first_call    = 0;
        ctx->SOC_precedent = SOC;
    } else {
        delta_SOC          = SOC - ctx->SOC_precedent;
        ctx->SOC_precedent = SOC;
    }

    /* Mise à jour du filtre de Kalman */
    estimation_RUL_core(ctx, SOH, delta_SOC);

    /* Sortie corrigée : RUL / vitesse_degradation (protégée) */
    float RUL_corrige = 0.0f;
    if (ctx->vitesse_degradation != 0.0f) {
        RUL_corrige = ctx->RUL_est / ctx->vitesse_degradation;
    }

    return RUL_corrige;
}
