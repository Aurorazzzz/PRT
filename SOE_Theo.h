#ifndef SOE_THEO_H
#define SOE_THEO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>  // size_t
#include <stdint.h>  // types entiers fixes

// -----------------------------------------------------------------------------
// Interpolation linéaire rapide 1D (équivalent MATLAB "interp1rapide")
// Entrées :
//   - x_tab : abscisses (ex : SOC normalisé 0..1)
//   - y_tab : ordonnées (ex : LOI_INTEG_OCV_DECHARGE)
//   - n     : taille des tableaux
//   - x     : valeur à interpoler
// Retour : valeur interpolée
// -----------------------------------------------------------------------------
float interp1rapide(const float *x_tab, const float *y_tab, int n, float x);

// -----------------------------------------------------------------------------
// Estimation du SOE (State Of Energy)
// SOE ≈ V̄_OCV(SOC) * (1/(moins_eta_sur_Q)) * SOH * SOC
// Entrées :
//   - SOC, SOH          : état de charge / santé (bornés 0..1)
//   - moins_eta_sur_Q   : inverse (rendement * capacité) [1/Coulomb]
//   - X_OCV             : abscisses de la table (SOC)
//   - LOI_INTEG_OCV_DECHARGE : valeurs intégrées (V̄_OCV ou équivalent)
//   - n                 : taille des tables
// Retour : SOE prédit (unités cohérentes avec la table et Q)
// -----------------------------------------------------------------------------
float estimation_SOE(float SOC, float SOH, float moins_eta_sur_Q,
                     const float *X_OCV, const float *LOI_INTEG_OCV_DECHARGE, int n);

// -----------------------------------------------------------------------------
// setup() : charge/initialise les données et exécute un calcul de démonstration
// (Déclarée ici si vous l’appelez depuis main.c)
// -----------------------------------------------------------------------------
void setup(void);

#ifdef __cplusplus
}
#endif

#endif // SOE_THEO_H
