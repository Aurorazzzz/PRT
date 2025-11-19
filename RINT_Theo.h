#ifndef RINT_THEO_H
#define RINT_THEO_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Estimation de RINT (traduction C du script MATLAB estimation_RINT)
// Entrées instantanées : tension, courant, SOC
// Paramètres : a_filtre_RINT[2], b_filtre_RINT[2] (IIR d'ordre 1)
// États in/out : RINT, SOHR, RINT_INIT, RINTkm1, RINTfiltrekm1,
//                tension_precedente, courant_precedent, compteur_RINT, RINT_ref
// Effet : met à jour tous les états (équivalent aux sorties MATLAB).
// ============================================================================
void estimation_RINT_step(float tension,
                          float courant,
                          float SOC,
                          const float a_filtre_RINT[2],
                          const float b_filtre_RINT[2],
                          double *RINT,
                          float *SOHR,
                          float *RINT_INIT,
                          double *RINTkm1,
                          double *RINTfiltrekm1,
                          float *tension_precedente,
                          float *courant_precedent,
                          float *compteur_RINT,
                          float *RINT_ref);

// Démo d’intégration : charge les buffers avec Charge_donnees, boucle sur N.
void setup(void);

#ifdef __cplusplus
}
#endif

#endif // RINT_THEO_H
