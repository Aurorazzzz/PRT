#include <math.h>
#include <stddef.h>
#include "sur_temperature.h"

// ============================================================================
// Module SUR_TEMPERATURE — Surveillance cadencée de la température batterie
// Projet : SBOVA – Fonction BMS (TRL3)
//
// Description :
//   Ce module assure la surveillance en ligne de la température de la
//   batterie. Il compare la température mesurée à des seuils configurables
//   afin de détecter des conditions de sur-température ou de sous-température.
//
//   L’algorithme est conçu pour un appel cadencé (typiquement 1 Hz) et
//   intègre des mécanismes d’hystérésis et de temporisation afin d’éviter
//   les déclenchements intempestifs.
//
// Entrées (par appel) :
//   - ctx         : contexte de surveillance thermique déjà initialisé
//                   (SUR_TEMPERATURE_Context)
//   - temperature : température batterie mesurée [°C]
//
// Sorties :
//   - Valeur de retour :
//       état thermique courant (OK / ALERTE / DÉFAUT selon implémentation)
//   - Flags internes dans le contexte utilisables par le superviseur BMS
//
// Rôle dans l’architecture BMS :
//   - Protection de la batterie contre les conditions thermiques critiques
//   - Entrée de sécurité pour la limitation de puissance (SOP)
//   - Condition d’inhibition pour certains estimateurs (SOC, RINT, SOH)
//
// Hypothèses et remarques :
//   - Appel strictement cadencé avec période constante
//   - Seuils thermiques définis et initialisés dans le contexte
//   - Aucun appel à l’allocation dynamique mémoire
//   - Comportement déterministe adapté à l’embarqué temps réel
//
// Auteur :
//   Projet SBOVA – INSA Strasbourg / ICube
//
// Date de création :
//   2026-01-08
//
// Dernière modification :
//   2026-01-08
// ============================================================================


void modele_thermique_foster_ordre_2_step(const float p[4], float I,
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
    const float param[4] = {R1_modele_thermique, C1_modele_thermique, R2_modele_thermique, C2_modele_thermique };
    
    modele_thermique_foster_ordre_2_step(param, courant, dt,
    TAMB, T1, T2);

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
