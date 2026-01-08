#ifndef SOP_STEP_H
#define SOP_STEP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct
{
    /* ================== Constantes (remplies dans SOP_init) ================== */
    float dt;
    int   horizon;

    float moins_eta_sur_Q;

    float therm[4];     /* R1 C1 R2 C2 */
    float TAMB;

    const float *X_OCV;
    const float *Y_OCV_charge;
    const float *Y_OCV_decharge;
    int          n_OCV;

    float R0, R1, C1_RC;

    float SOC_min, SOC_max;
    float U_min, U_max;
    float T_max;
    float I_min, I_max;

    float Ki_T_decharge, Kp_T_decharge;
    float Ki_T_charge,   Kp_T_charge;
    float Ki_Umax,       Kp_Umax;
    float Ki_Umin,       Kp_Umin;

    /* ================== États dynamiques ================== */
    float SOC;
    float SOH;
    float T1;
    float T2;
    float Ir;

    float U_km1;
    float U_km2;
    float T2_km1;

    float I_candidat;

    float tampon_I[60];
    int   etat; /* 0=charge, 1=decharge */

} SOP_Context;


// init : vous donnez conditions initiales (SOC/SOH/T1/T2/Ir + tension init)
void SOP_init(SOP_Context *ctx,
              float SOC0, float SOH0,
              float T1_0, float T2_0,
              float Ir0,
              float U0);

// optionnel : si SOH vient d’un autre module step
static inline void SOP_set_SOH(SOP_Context *ctx, float SOH)
{
    if (!ctx) return;
    ctx->SOH = SOH;
}

// optionnel : si vous voulez forcer SOC (ex : venant de votre estimation SOC)
static inline void SOP_set_SOC(SOP_Context *ctx, float SOC)
{
    if (!ctx) return;
    ctx->SOC = SOC;
}

// tick : entrée = courant demandé (consigne).
// sortie = courant limité (ce que vous appliquez réellement) + SOP charge/décharge (puissance) + etat.
float SOP_step(SOP_Context *ctx,
               float I_consigne,
               float *SOP_charge_W,
               float *SOP_decharge_W,
               int   *etat_out);

#ifdef __cplusplus
}
#endif

#endif
