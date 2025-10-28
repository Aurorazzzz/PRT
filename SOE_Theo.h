#ifndef ESTIMATION_SOE_H
#define ESTIMATION_SOE_H

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// estimation_SOE.h
// Déclaration des fonctions pour le calcul du SOE (State Of Energy)
// Projet : SBOVA – Estimation de paramètres batterie
// Auteur : schmitt Théo
// Date   : 28/10/2025
// ============================================================================

#include <stddef.h>

// ---------------------------------------------------------------------------
// Fonction : interp1rapide
// Description : interpolation linéaire rapide entre deux points de la table.
// Entrées :
//   - x_tab : tableau des abscisses (X_OCV)
//   - y_tab : tableau des ordonnées (LOI_INTEG_OCV_DECHARGE)
//   - n     : taille du tableau
//   - x     : valeur à interpoler (ex: SOC)
// Sortie :
//   - valeur interpolée
// ---------------------------------------------------------------------------
float interp1rapide(float *x_tab, float *y_tab, int n, float x);

// ---------------------------------------------------------------------------
// Fonction : estimation_SOE
// Description : calcule le State Of Energy (SOE) à partir du SOC et du SOH.
// Entrées :
//   - SOC, SOH          : état de charge et état de santé (entre 0 et 1)
//   - moins_eta_sur_Q   : inverse du produit (rendement * capacité) [1/C]
//   - X_OCV             : abscisses de la table d’OCV moyenne
//   - LOI_INTEG_OCV_DECHARGE : valeurs intégrées de la loi OCV (en V)
//   - n                 : taille des tables
// Sortie :
//   - SOE_pred          : énergie disponible (en Joules si cohérence des unités)
// ---------------------------------------------------------------------------
float estimation_SOE(float SOC, float SOH, float moins_eta_sur_Q,
                     float *X_OCV, float *LOI_INTEG_OCV_DECHARGE, int n);

#ifdef __cplusplus
}
#endif

#endif // ESTIMATION_SOE_H
