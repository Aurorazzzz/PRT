#ifndef SOP_H
#define SOP_H

#include <stddef.h>

/**
 * Calcule les SOP de charge et de décharge sur tout l’horizon de simulation.
 *
 * Entrées :
 *   courant, tension, temperature, SOH, SOC  : signaux d’entrée (taille N)
 *   N                                        : nombre d’échantillons
 *
 *   moins_eta_sur_Q                          : 1/(rendement * Q) (même valeur que dans vos autres modules)
 *   coeffs_thermique[4]                      : {R1, C1, R2, C2} du modèle Foster d’ordre 2
 *   TAMB                                     : température ambiante
 *   T1_init, T2_init                         : conditions initiales du modèle thermique
 *
 *   X_OCV, Y_OCV_charge, Y_OCV_decharge      : tables OCV (mêmes que surveillance_tension)
 *   n_OCV                                    : taille des tables OCV
 *   R0, R1, C1_RC                            : paramètres du modèle tension 1RC
 *
 *   SOC_min, SOC_max, U_min, U_max, T_max    : contraintes BMS
 *   I_min, I_max                             : limites courant
 *
 * Sorties :
 *   SOP_charge[i]   : SOP de charge à l’instant i (W)
 *   SOP_decharge[i] : SOP de décharge à l’instant i (W)
 */
void SOP_predictif(
    const float *courant,
    const float *tension,
    const float *temperature,
    const float *SOH,
    const float *SOC,
    size_t       N,

    float moins_eta_sur_Q,
    const float coeffs_thermique[4],
    float TAMB,
    float T1_init,
    float T2_init,

    const float *X_OCV,
    const float *Y_OCV_charge,
    const float *Y_OCV_decharge,
    int          n_OCV,
    float R0,
    float R1,
    float C1_RC,

    float SOC_min,
    float SOC_max,
    float U_min,
    float U_max,
    float T_max,
    float I_min,
    float I_max,

    float *SOP_charge,
    float *SOP_decharge
);

/* Optionnel : programme autonome comme SOE_Theo, SURVEILLANCE, RUL… */
void setup_SOP(void);

#endif /* SOP_H */
