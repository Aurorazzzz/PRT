#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "SOP.h"

static const float X_OCV_global[] = {
    0.0f, 0.02f, 0.04f, 0.06f, 0.08f, 0.15f, 0.21f, 0.30f, 0.40f, 0.50f,
    0.60f, 0.70f, 0.80f, 0.85f, 0.90f, 0.92f, 0.93f, 0.95f, 0.97f, 0.99f, 1.0f
};

static const float Y_OCV_charge_global[] = {
    2.74615136878047f, 2.94688704780013f, 3.04478208162645f, 3.11302386632185f,
    3.14303727005486f, 3.18759401184296f, 3.22099078113839f, 3.24905171403132f,
    3.27011839999992f, 3.28534786027309f, 3.29979224246840f, 3.31641533857934f,
    3.33973986043135f, 3.35630007692910f, 3.38039228328910f, 3.39594494709583f,
    3.40676702687123f, 3.44012152068810f, 3.48795254849592f, 3.57441784707680f,
    3.60655075278407f
};

static const float Y_OCV_decharge_global[] = {
    2.746151368780468f, 2.946887047800132f, 3.044782081626454f, 3.113023866321848f,
    3.143037270054857f, 3.187594011842960f, 3.220990781138394f, 3.249051714031321f,
    3.270118399999917f, 3.285347860273093f, 3.299792242468398f, 3.316415338579337f,
    3.339739860431349f, 3.350334634002046f, 3.360929407572744f, 3.365167317001023f,
    3.367286271715163f, 3.371524181143442f, 3.375762090571721f, 3.380000000000000f,
    3.606550752784074f
};

/* ===================== helpers (copiés/alignés) ===================== */

static void interp1rapide_der(const float *x, const float *y, int n,
                              float x_req, float *sortie, float *der)
{
    int indice = -1;
    for (int i = 0; i < n; ++i) {
        if (x_req > x[i]) indice = i;
    }

    float coord;
    if (indice >= 0 && indice < n - 1) {
        float dx = x[indice + 1] - x[indice];
        if (dx == 0.0f) dx = 1e-9f;
        coord = (x_req - x[indice]) / dx;
        *der  = (y[indice + 1] - y[indice]) / dx;
    } else if (indice < 0) {
        indice = 0;
        float dx = x[1] - x[0];
        if (dx == 0.0f) dx = 1e-9f;
        coord = 0.0f;
        *der  = (y[1] - y[0]) / dx;
    } else {
        indice = n - 2;
        float dx = x[indice + 1] - x[indice];
        if (dx == 0.0f) dx = 1e-9f;
        coord = 1.0f;
        *der  = (y[indice + 1] - y[indice]) / dx;
    }

    *sortie = (1.0f - coord) * y[indice] + coord * y[indice + 1];
}

static float modele_SOC_CC_step(float moins_eta_sur_Q, float dt,
                                float SOC_prev, float I, float SOH)
{
    if (SOH == 0.0f) return SOC_prev;
    return SOC_prev - moins_eta_sur_Q * dt * I / SOH;
}

static void modele_thermique_foster_ordre_2_step(const float p[4], float I,
                                                 float dt, float TAMB,
                                                 float *T1, float *T2)
{
    float R1 = p[0], C1 = p[1], R2 = p[2], C2 = p[3];
    float T1_prev = *T1;
    float T2_prev = *T2;

    float T1_inst = T1_prev + dt * (R1 * I * I + TAMB - T1_prev) / (R1 * C1);
    float T2_inst = T2_prev + dt * (T1_prev - T2_prev) / (R2 * C2);

    *T1 = T1_inst;
    *T2 = T2_inst;
}

static float modele_tension_1RC_step(float I, float SOC, float *Ir,
                                     int etat,
                                     const float *X_OCV,
                                     const float *Y_OCV_charge,
                                     const float *Y_OCV_decharge,
                                     int n_OCV,
                                     float dt, float R1, float C1, float R0)
{
    float denom = R1 * C1;
    if (denom != 0.0f) {
        float alpha = -dt / denom + 1.0f;
        float beta  =  dt / denom;
        *Ir = alpha * (*Ir) + beta * I;
    }

    if (SOC < 0.0f) SOC = 0.0f;
    if (SOC > 1.0f) SOC = 1.0f;

    const float *Y_tab = etat ? Y_OCV_decharge : Y_OCV_charge;

    float OCV, der_dummy;
    interp1rapide_der(X_OCV, Y_tab, n_OCV, SOC, &OCV, &der_dummy);

    return OCV - R1 * (*Ir) - R0 * I;
}

/* ===================== simulation horizon (min/max) ===================== */

static void simuler_horizon_batterie(float moins_eta_sur_Q, float dt, int horizon,
                                     float SOC_init, float SOH,
                                     const float therm[4],
                                     float T1_init, float T2_init,
                                     float TAMB, float Ir_init, int etat,
                                     const float *X_OCV,
                                     const float *Y_OCV_charge,
                                     const float *Y_OCV_decharge,
                                     int n_OCV,
                                     float R1, float C1, float R0,
                                     float I_cand,
                                     float SOC_minmax[2],
                                     float T2_minmax[2],
                                     float U_minmax[2])
{
    float SOC = SOC_init;
    float T1  = T1_init;
    float T2  = T2_init;
    float Ir  = Ir_init;

    SOC_minmax[0] = SOC; SOC_minmax[1] = SOC;
    T2_minmax[0]  = T2;  T2_minmax[1]  = T2;

    float U0 = modele_tension_1RC_step(I_cand, SOC, &Ir, etat,
                                       X_OCV, Y_OCV_charge, Y_OCV_decharge, n_OCV,
                                       dt, R1, C1, R0);
    U_minmax[0] = U0; U_minmax[1] = U0;

    for (int k = 1; k < horizon; ++k) {
        SOC = modele_SOC_CC_step(moins_eta_sur_Q, dt, SOC, I_cand, SOH);
        modele_thermique_foster_ordre_2_step(therm, I_cand, dt, TAMB, &T1, &T2);
        float U = modele_tension_1RC_step(I_cand, SOC, &Ir, etat,
                                          X_OCV, Y_OCV_charge, Y_OCV_decharge, n_OCV,
                                          dt, R1, C1, R0);

        if (SOC < SOC_minmax[0]) SOC_minmax[0] = SOC;
        if (SOC > SOC_minmax[1]) SOC_minmax[1] = SOC;

        if (T2 < T2_minmax[0]) T2_minmax[0] = T2;
        if (T2 > T2_minmax[1]) T2_minmax[1] = T2;

        if (U < U_minmax[0]) U_minmax[0] = U;
        if (U > U_minmax[1]) U_minmax[1] = U;
    }
}

/* ===================== racine Pegase (version step-friendly) ===================== */

static float recherche_racine_SOP_Pegase_1RC_step(
    float moins_eta_sur_Q, float dt, int horizon,
    float SOC_actuel, float SOH_actuel,
    const float therm[4],
    float T1_init, float T2_init, float TAMB,
    float SOC_min, float SOC_max,
    float U_min, float U_max,
    float T_max,
    float I_min, float I_max,
    float consigne_courant,
    float Ir_init,
    int   etat,
    const float *X_OCV,
    const float *Y_OCV_charge,
    const float *Y_OCV_decharge,
    int n_OCV,
    float R1, float C1, float R0,
    float courant_requete,
    float residus_out[3]
){
    bool coteA = false, coteB = false;

    float borne_A, borne_B;
    if (consigne_courant > 0.0f) { borne_A = 0.0f; borne_B = I_max; }
    else                         { borne_A = 0.0f; borne_B = I_min; }

    // ---- borne A ----
    float SOCmmA[2], T2mmA[2], UmmA[2];
    simuler_horizon_batterie(moins_eta_sur_Q, dt, horizon,
                             SOC_actuel, SOH_actuel,
                             therm,
                             T1_init, T2_init, TAMB,
                             Ir_init, etat,
                             X_OCV, Y_OCV_charge, Y_OCV_decharge, n_OCV,
                             R1, C1, R0,
                             borne_A,
                             SOCmmA, T2mmA, UmmA);

    float resA[3];
    if (consigne_courant > 0.0f) {
        resA[0] = SOC_min - SOCmmA[0];
        resA[1] = T2mmA[1] - T_max;
        resA[2] = U_min - UmmA[0];
    } else {
        resA[0] = SOCmmA[1] - SOC_max;
        resA[1] = T2mmA[1] - T_max;
        resA[2] = UmmA[1] - U_max;
    }

    if (resA[0] > 0.0f || resA[1] > 0.0f || resA[2] > 0.0f) {
        if (residus_out) { residus_out[0]=resA[0]; residus_out[1]=resA[1]; residus_out[2]=resA[2]; }
        return borne_A; // 0
    }

    // ---- borne B ----
    float SOCmmB[2], T2mmB[2], UmmB[2];
    simuler_horizon_batterie(moins_eta_sur_Q, dt, horizon,
                             SOC_actuel, SOH_actuel,
                             therm,
                             T1_init, T2_init, TAMB,
                             Ir_init, etat,
                             X_OCV, Y_OCV_charge, Y_OCV_decharge, n_OCV,
                             R1, C1, R0,
                             borne_B,
                             SOCmmB, T2mmB, UmmB);

    float resB[3];
    if (consigne_courant > 0.0f) {
        resB[0] = SOC_min - SOCmmB[0];
        resB[1] = T2mmB[1] - T_max;
        resB[2] = U_min - UmmB[0];
    } else {
        resB[0] = SOCmmB[1] - SOC_max;
        resB[1] = T2mmB[1] - T_max;
        resB[2] = UmmB[1] - U_max;
    }

    if (resB[0] < 0.0f && resB[1] < 0.0f && resB[2] < 0.0f) {
        if (residus_out) { residus_out[0]=resB[0]; residus_out[1]=resB[1]; residus_out[2]=resB[2]; }
        return borne_B;
    }

    // ---- secante / Pegase (12 iters) ----
    float courant_final = courant_requete;

    for (int iter = 0; iter < 12; ++iter) {
        float borne_C;

        if (iter == 0) {
            borne_C = courant_requete;
        } else {
            float pas_vecteur[3];
            for (int k = 0; k < 3; ++k) {
                float pente = (resB[k] - resA[k]) / (borne_B - borne_A);
                if (pente == 0.0f) pente = 1e-9f;
                pas_vecteur[k] = -resA[k] / pente;

                if (resA[k] < 0.0f && resB[k] < 0.0f)
                    pas_vecteur[k] = borne_B - borne_A;
            }

            float pas = pas_vecteur[0];
            float absmin = fabsf(pas_vecteur[0]);
            for (int k = 1; k < 3; ++k) {
                float a = fabsf(pas_vecteur[k]);
                if (a < absmin) { absmin = a; pas = pas_vecteur[k]; }
            }

            if (consigne_courant <= 0.0f) borne_C = fmaxf(borne_A + pas, I_min);
            else                          borne_C = fminf(borne_A + pas, I_max);
        }

        float SOCmmC[2], T2mmC[2], UmmC[2];
        simuler_horizon_batterie(moins_eta_sur_Q, dt, horizon,
                                 SOC_actuel, SOH_actuel,
                                 therm,
                                 T1_init, T2_init, TAMB,
                                 Ir_init, etat,
                                 X_OCV, Y_OCV_charge, Y_OCV_decharge, n_OCV,
                                 R1, C1, R0,
                                 borne_C,
                                 SOCmmC, T2mmC, UmmC);

        float resC[3];
        if (consigne_courant > 0.0f) {
            resC[0] = SOC_min - SOCmmC[0];
            resC[1] = T2mmC[1] - T_max;
            resC[2] = U_min - UmmC[0];
        } else {
            resC[0] = SOCmmC[1] - SOC_max;
            resC[1] = T2mmC[1] - T_max;
            resC[2] = UmmC[1] - U_max;
        }

        bool violation = (resC[0] > 0.0f || resC[1] > 0.0f || resC[2] > 0.0f);

        courant_final = borne_C;
        if (residus_out) { residus_out[0]=resC[0]; residus_out[1]=resC[1]; residus_out[2]=resC[2]; }

        if (violation) {
            borne_B = borne_C;
            for (int k = 0; k < 3; ++k) resB[k] = resC[k];

            if (coteA) {
                for (int k = 0; k < 3; ++k) {
                    float denom = (resA[k] + resC[k]);
                    if (denom == 0.0f) denom = 1e-9f;
                    float gamma = resA[k] / denom;
                    resB[k] *= gamma;
                }
            }
            coteA = true; coteB = false;
        } else {
            borne_A = borne_C;
            for (int k = 0; k < 3; ++k) resA[k] = resC[k];

            if (coteB) {
                for (int k = 0; k < 3; ++k) {
                    float denom = (resB[k] + resC[k]);
                    if (denom == 0.0f) denom = 1e-9f;
                    float gamma = resB[k] / denom;
                    resA[k] *= gamma;
                }
            }
            coteB = true; coteA = false;
        }
    }

    // saturation finale par rapport à la consigne
    if (consigne_courant <= 0.0f) courant_final = fmaxf(consigne_courant, courant_final);
    else                          courant_final = fminf(consigne_courant, courant_final);

    return courant_final;
}

/* ===================== init / step publics ===================== */

void SOP_init(SOP_Context *ctx,
              float SOC0, float SOH0,
              float T10, float T20,
              float Ir0,
              float U0)
{
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));

    /* ================== Constantes (depuis votre setup_SOP) ================== */
    ctx->dt      = 1.0f;
    ctx->horizon = 30;

    ctx->moins_eta_sur_Q = 2.3003039e-4f;

    ctx->therm[0] = 0.206124119186158f;
    ctx->therm[1] = 50.3138901982787f;
    ctx->therm[2] = 21.6224372540937f;
    ctx->therm[3] = 15.8943772584241f;

    ctx->TAMB = 25.0f;

    ctx->X_OCV        = X_OCV_global;
    ctx->Y_OCV_charge = Y_OCV_charge_global;
    ctx->Y_OCV_decharge = Y_OCV_decharge_global;
    ctx->n_OCV = 21;

    ctx->R0    = 0.022140255136947f;
    ctx->R1    = 0.018585867413143f;
    ctx->C1_RC = 8.252903566971308e+2f;

    ctx->SOC_min = 0.1f;
    ctx->SOC_max = 0.9f;
    ctx->U_min   = 2.0f;
    ctx->U_max   = 3.6f;
    ctx->T_max   = 60.0f;
    ctx->I_min   = -20.0f;
    ctx->I_max   = 20.0f;

    ctx->Ki_T_decharge =  1.0f; ctx->Kp_T_decharge =  5.0f;
    ctx->Ki_T_charge   = -1.0f; ctx->Kp_T_charge   = -5.0f;
    ctx->Ki_Umax       = -5.0f; ctx->Kp_Umax       = -5.0f;
    ctx->Ki_Umin       = -5.0f; ctx->Kp_Umin       = -5.0f;

    /* ================== États init ================== */
    ctx->SOC = SOC0;
    ctx->SOH = SOH0;
    ctx->T1  = T10;
    ctx->T2  = T20;
    ctx->Ir  = Ir0;

    ctx->U_km2  = U0;
    ctx->U_km1  = U0;
    ctx->T2_km1 = T20;

    ctx->I_candidat = 0.0f;

    for (int i = 0; i < 60; ++i) ctx->tampon_I[i] = 0.0f;
    ctx->etat = 0;
}

static void detection_phase_step(SOP_Context *ctx, float I)
{
    // decalage
    memmove(&ctx->tampon_I[1], &ctx->tampon_I[0], (60 - 1) * sizeof(float));
    ctx->tampon_I[0] = I;

    float somme = 0.0f;
    for (int k = 0; k < 60; ++k) somme += ctx->tampon_I[k];
    float moyenne = somme / 60.0f;

    int etat_prec = ctx->etat;
    if (moyenne > 0.1f && etat_prec == 0)      ctx->etat = 1;
    else if (moyenne < -1.0f && etat_prec == 1) ctx->etat = 0;
}

float SOP_step(SOP_Context *ctx,
               float I_consigne,
               float *SOP_charge_W,
               float *SOP_decharge_W,
               int   *etat_out)
{
    if (!ctx) return 0.0f;

    // 1) détection charge/décharge sur la consigne
    detection_phase_step(ctx, I_consigne);
    if (etat_out) *etat_out = ctx->etat;

    // 2) limitation prédictive (Pegase) -> courant_resultat
    float residus[3] = {0,0,0};

    float I_predictif = recherche_racine_SOP_Pegase_1RC_step(
        ctx->moins_eta_sur_Q, ctx->dt, ctx->horizon,
        ctx->SOC, ctx->SOH,
        ctx->therm,
        ctx->T1, ctx->T2, ctx->TAMB,
        ctx->SOC_min, ctx->SOC_max,
        ctx->U_min,   ctx->U_max,
        ctx->T_max,
        ctx->I_min, ctx->I_max,
        I_consigne,
        ctx->Ir,
        ctx->etat,
        ctx->X_OCV, ctx->Y_OCV_charge, ctx->Y_OCV_decharge, ctx->n_OCV,
        ctx->R1, ctx->C1_RC, ctx->R0,
        I_consigne,
        residus
    );

    // sécurité : ne pas dépasser la consigne
    if (I_consigne > 0.0f) {
        if (I_predictif > I_consigne) I_predictif = I_consigne;
    } else {
        if (I_predictif < I_consigne) I_predictif = I_consigne;
    }

    // 3) limitation instantanée (comme votre boucle) -> delta_I_candidat
    float delta_I_SOCmax, delta_I_SOCmin, delta_I_Umax, delta_I_Umin;
    float delta_I_Tmax_charge, delta_I_Tmax_decharge;
    float delta_I_Imax, delta_I_Imin;

    // SOCmax
    if (ctx->SOC >= ctx->SOC_max) delta_I_SOCmax = -ctx->I_candidat;
    else                         delta_I_SOCmax = ctx->I_min - ctx->I_candidat;

    // Umax
    {
        float erreur_Umax = ctx->U_max - ctx->U_km1;
        float der_erreur_Umax = -(ctx->U_km1 - ctx->U_km2) / ctx->dt;
        delta_I_Umax = ctx->Ki_Umax * erreur_Umax + ctx->Kp_Umax * der_erreur_Umax;
        float max_delta_for_zero = -ctx->I_candidat;
        if (delta_I_Umax > max_delta_for_zero) delta_I_Umax = max_delta_for_zero;
    }

    // SOCmin
    if (ctx->SOC <= ctx->SOC_min) delta_I_SOCmin = -ctx->I_candidat;
    else                         delta_I_SOCmin = ctx->I_max - ctx->I_candidat;

    // Umin
    {
        float erreur_Umin = ctx->U_min - ctx->U_km1;
        float der_erreur_Umin = -(ctx->U_km1 - ctx->U_km2) / ctx->dt;
        delta_I_Umin = ctx->Ki_Umin * erreur_Umin + ctx->Kp_Umin * der_erreur_Umin;
        float min_delta = -ctx->I_candidat;
        if (delta_I_Umin < min_delta) delta_I_Umin = min_delta;
    }

    // Tmax
    {
        float erreur_Tmax = ctx->T_max - ctx->T2;
        float der_erreur_Tmax = -(ctx->T2 - ctx->T2_km1) / ctx->dt;
        delta_I_Tmax_charge   = ctx->Ki_T_charge   * erreur_Tmax + ctx->Kp_T_charge   * der_erreur_Tmax;
        delta_I_Tmax_decharge = ctx->Ki_T_decharge * erreur_Tmax + ctx->Kp_T_decharge * der_erreur_Tmax;

        float max_delta_charge = -ctx->I_candidat;
        if (delta_I_Tmax_charge > max_delta_charge) delta_I_Tmax_charge = max_delta_charge;

        float min_delta_decharge = -ctx->I_candidat;
        if (delta_I_Tmax_decharge < min_delta_decharge) delta_I_Tmax_decharge = min_delta_decharge;
    }

    delta_I_Imax = ctx->I_max - ctx->I_candidat;
    delta_I_Imin = ctx->I_min - ctx->I_candidat;

    // 4) calcul SOP (puissance) : courant critique * tension (U_km1)
    {
        float I_lim_Imin        = ctx->I_candidat + delta_I_Imin;
        float I_lim_SOCmax      = ctx->I_candidat + delta_I_SOCmax;
        float I_lim_Umax        = ctx->I_candidat + delta_I_Umax;
        float I_lim_Tmax_charge = ctx->I_candidat + delta_I_Tmax_charge;

        float Icrit_charge = I_lim_Imin;
        if (I_lim_SOCmax      > Icrit_charge) Icrit_charge = I_lim_SOCmax;
        if (I_lim_Umax        > Icrit_charge) Icrit_charge = I_lim_Umax;
        if (I_lim_Tmax_charge > Icrit_charge) Icrit_charge = I_lim_Tmax_charge;

        float I_lim_Imax           = ctx->I_candidat + delta_I_Imax;
        float I_lim_SOCmin         = ctx->I_candidat + delta_I_SOCmin;
        float I_lim_Umin           = ctx->I_candidat + delta_I_Umin;
        float I_lim_Tmax_decharge  = ctx->I_candidat + delta_I_Tmax_decharge;

        float Icrit_decharge = I_lim_Imax;
        if (I_lim_SOCmin        < Icrit_decharge) Icrit_decharge = I_lim_SOCmin;
        if (I_lim_Umin          < Icrit_decharge) Icrit_decharge = I_lim_Umin;
        if (I_lim_Tmax_decharge < Icrit_decharge) Icrit_decharge = I_lim_Tmax_decharge;

        if (SOP_charge_W)   *SOP_charge_W   = Icrit_charge   * ctx->U_km1;
        if (SOP_decharge_W) *SOP_decharge_W = Icrit_decharge * ctx->U_km1;
    }

    // 5) sélection du delta_I (priorités identiques à votre code)
    float delta_I_consigne = I_predictif - ctx->I_candidat;
    float delta_I_candidat = delta_I_consigne;

    if (delta_I_consigne < delta_I_SOCmax) delta_I_candidat = delta_I_SOCmax;
    if (delta_I_candidat < delta_I_Umax)   delta_I_candidat = delta_I_Umax;
    if (delta_I_candidat < delta_I_Tmax_charge) delta_I_candidat = delta_I_Tmax_charge;

    if (delta_I_candidat > delta_I_SOCmin) delta_I_candidat = delta_I_SOCmin;
    if (delta_I_candidat > delta_I_Umin)   delta_I_candidat = delta_I_Umin;
    if (delta_I_candidat > delta_I_Tmax_decharge) delta_I_candidat = delta_I_Tmax_decharge;

    if (delta_I_candidat > delta_I_Imax) delta_I_candidat = delta_I_Imax;
    if (delta_I_candidat < delta_I_Imin) delta_I_candidat = delta_I_Imin;

    // 6) mise à jour courant candidat appliqué
    ctx->I_candidat = ctx->I_candidat + ctx->dt * delta_I_candidat;

    // 7) “plante” le système d’un pas avec I_candidat (mise à jour SOC, T, U, Ir)
    {
        float SOC_new = modele_SOC_CC_step(ctx->moins_eta_sur_Q, ctx->dt, ctx->SOC, ctx->I_candidat, ctx->SOH);

        float T1_new = ctx->T1;
        float T2_new = ctx->T2;
        modele_thermique_foster_ordre_2_step(ctx->therm, ctx->I_candidat, ctx->dt, ctx->TAMB, &T1_new, &T2_new);

        float Ir_new = ctx->Ir;
        float U_new  = modele_tension_1RC_step(ctx->I_candidat, SOC_new, &Ir_new,
                                               ctx->etat,
                                               ctx->X_OCV, ctx->Y_OCV_charge, ctx->Y_OCV_decharge, ctx->n_OCV,
                                               ctx->dt, ctx->R1, ctx->C1_RC, ctx->R0);

        // mémoires dérivées
        ctx->U_km2 = ctx->U_km1;
        ctx->U_km1 = U_new;

        ctx->T2_km1 = ctx->T2;

        // états
        ctx->SOC = SOC_new;
        ctx->T1  = T1_new;
        ctx->T2  = T2_new;
        ctx->Ir  = Ir_new;
    }

    // courant réellement appliqué
    return ctx->I_candidat;
}
