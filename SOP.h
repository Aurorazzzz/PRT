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

 static void interp1rapide_der(const float *x,
                              const float *y,
                              int          n,
                              float        x_req,
                              float       *sortie,
                              float       *der);

void simuler_horizon_batterie(float moins_eta_sur_Q,
                              float dt,
                              int   horizon,
                              float SOC_init,
                              float SOH,
                              const float parametre_therm[4],
                              float T1_init,
                              float T2_init,
                              float TAMB,
                              float Ir_init,
                              int   etat,
                              const float *X_OCV_dep,
                              const float *Y_OCV_dep_charge,
                              const float *Y_OCV_dep_decharge,
                              int   n_OCV,
                              float R1,
                              float C1,
                              float R0,
                              float I_candidat,
                              float SOC_minmax[2], // [min, max]
                              float T1_minmax[2],
                              float T2_minmax[2],
                              float U_minmax[2],
                              float *Ir_final);

static float modele_SOC_CC_step(float moins_eta_sur_Q,
                                float dt,
                                float SOC_prev,
                                float I,
                                float SOH);

static void modele_thermique_foster_ordre_2_step(const float parametre[4],
                                                 float       I,
                                                 float       dt,
                                                 float       TAMB,
                                                 float      *T1,   // in/out
                                                 float      *T2);  // in/out

static float modele_tension_1RC_step(float        I,
                                     float        SOC,
                                     float       *Ir,   // in/out
                                     int          etat, // 1 = décharge, 0 = charge
                                     const float *X_OCV_dep,
                                     const float *Y_OCV_dep_charge,
                                     const float *Y_OCV_dep_decharge,
                                     int          n_OCV,
                                     float        dt,
                                     float        R1,
                                     float        C1,
                                     float        R0);
    
    
    
static float recherche_racine_SOP_Pegase_1RC(
    float moins_eta_sur_Q,
    float dt,
    int   horizon,
    float SOC_actuel,
    float SOH_actuel,
    const float *coefficients_modele_temperature, /* taille 4 */
    float T1_init,
    float temperature_actuelle,
    float TAMB,
    float SOC_min,
    float SOC_max,
    float U_min,
    float U_max,
    float T_max,
    float I_min,
    float I_max,
    float consigne_courant,      /* courant(i) dans le script */
    float Ir_init,
    int   etat,
    const float *X_OCV_dep,
    const float *Y_OCV_dep_charge,
    const float *Y_OCV_dep_decharge,
    int   n_OCV,
    float R1,
    float C1,
    float R0,
    float courant_requete, 
    float *residus        /* courant(i) dans le script */
);

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

void setup_SOP(void);

#endif /* SOP_H */
