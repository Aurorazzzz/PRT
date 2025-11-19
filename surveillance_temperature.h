#ifndef SURVEILLANCE_TEMPERATURE_H
#define SURVEILLANCE_TEMPERATURE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Prototype de la fonction principale de surveillance thermique
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
                              int *alerte);

// ============================================================================
// Prototype du setup (programme autonome)
// ============================================================================
void setup(void);

#ifdef __cplusplus
}
#endif

#endif // SURVEILLANCE_TEMPERATURE_H
