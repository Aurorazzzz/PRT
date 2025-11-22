#ifndef SURVEILLANCE_TENSION_H
#define SURVEILLANCE_TENSION_H

#include <stdio.h>

// ============================================================================
// Interpolation linéaire 1D (équivalent MATLAB interp1)
// ============================================================================
float interp1rapide(const float *x_tab,
                    const float *y_tab,
                    int n,
                    float x);

// ============================================================================
// Détection de phase charge/décharge (reproduction strict de MATLAB)
// ----------------------------------------------------------------------------
// état = 0 --> charge
// état = 1 --> décharge
// ============================================================================
void detection_phase_charge_decharge(float courant,
                                     float tampon_charge_decharge[60],
                                     int *etat,
                                     int *etat_precedent);

// ============================================================================
// Modèle de tension dynamique
// (équivalent MATLAB surveillance_tension.m)
// ============================================================================
void surveillance_tension(float courant,
                          float SOC,
                          float tension_mesuree,
                          int etat,  // 0 = charge, 1 = décharge
                          const float *X_OCV,
                          const float *Y_OCV_charge,
                          const float *Y_OCV_decharge,
                          int n_OCV,
                          float dt,
                          float R1,
                          float C1,
                          float R0,
                          float seuil_alerte,
                          float *Ir,
                          float *U,
                          int *alerte);

void TENSION_setup(void);

#endif // SURVEILLANCE_TENSION_H
