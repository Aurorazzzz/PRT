#include <math.h>
#include <stddef.h>
#include "sur_temperature.h"

// ============================================================================
// Fonction principale : surveillance température (équivalent MATLAB)
//
// Cette fonction est "step" : elle traite UN échantillon (courant, température)
// et met à jour directement T1 et T2.
// ============================================================================

void surveillance_temperature(float courant,
                              float temperature,
                              float *T1,
                              float *T2,
                              float R1_modele_thermique,
                              float C1_modele_thermique,
                              float R2_modele_thermique,
                              float C2_modele_thermique,
                              float seuil_alerte_temperature,
                              float TAMB,
                              float dt,
                              int *alerte)
{
    // Modèle thermique Foster d'ordre 2
    *T1 = *T1 + dt * (R1_modele_thermique * courant * courant +
                      TAMB - *T1) /
                      (R1_modele_thermique * C1_modele_thermique);

    *T2 = *T2 + dt * (*T1 - *T2) /
                      (R2_modele_thermique * C2_modele_thermique);

    // Détection d'écart température mesurée vs modèle
    float diff = fabsf(temperature - *T2);
    *alerte   = (diff > seuil_alerte_temperature) ? 1 : 0;
}

// ============================================================================
// Gestion du contexte : TEMP_init / TEMP_step
// ============================================================================

void TEMP_init(TEMP_Context *ctx)
{
    if (!ctx) return;

    // Paramètres EXACTS du modèle thermique MATLAB
    ctx->R1 = 0.206124119186158f;
    ctx->C1 = 50.3138901982787f;
    ctx->R2 = 21.6224372540937f;
    ctx->C2 = 15.8943772584241f;

    ctx->seuil_alerte_temperature = 10.0f;  // conforme MATLAB
    ctx->TAMB                     = 25.0f;  // conforme MATLAB
    ctx->dt                       = 1.0f;   // conforme MATLAB

    // Conditions initiales MATLAB
    ctx->T1 = 60.0f;
    ctx->T2 = 30.0f;
}

float TEMP_step(TEMP_Context *ctx,
                float courant,
                float temperature,
                int  *alerte)
{
    if (!ctx) return 0.0f;

    int alerte_local = 0;
    int *p_alerte = (alerte != NULL) ? alerte : &alerte_local;

    surveillance_temperature(
        courant,
        temperature,
        &ctx->T1,
        &ctx->T2,
        ctx->R1,
        ctx->C1,
        ctx->R2,
        ctx->C2,
        ctx->seuil_alerte_temperature,
        ctx->TAMB,
        ctx->dt,
        p_alerte
    );

    return ctx->T2;  
}
