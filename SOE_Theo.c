#include <stdio.h>
#include "SOE_Theo.h"

// ============================================================================
// Interpolation linéaire rapide 1D : équivalent de "interp1rapide" MATLAB
// Entrées :
//   x_tab[] : tableau des abscisses (X_OCV)
//   y_tab[] : tableau des ordonnées (LOI_INTEG_OCV_DECHARGE)
//   n       : taille du tableau
//   x       : valeur d’interpolation (ici le SOC)
// Sortie : valeur interpolée
// ============================================================================
float interp1rapide(float *x_tab, float *y_tab, int n, float x)
{
    if (x <= x_tab[0]) return y_tab[0];
    if (x >= x_tab[n - 1]) return y_tab[n - 1];

    for (int i = 0; i < n - 1; i++) {
        if (x >= x_tab[i] && x <= x_tab[i + 1]) {
            float dx = x_tab[i + 1] - x_tab[i];
            float dy = y_tab[i + 1] - y_tab[i];
            float t = (x - x_tab[i]) / dx;
            return y_tab[i] + t * dy;
        }
    }
    return y_tab[n - 1];  // sécurité
}

// ============================================================================
// Fonction principale : estimation du SOE
// Entrées :
//   SOC, SOH                : état de charge et de santé (0–1)
//   moins_eta_sur_Q         : 1/(rendement*capacité) [1/Coulomb]
//   X_OCV[], LOI_INTEG_OCV_DECHARGE[] : tables du modèle
//   n                       : taille de ces tables
// Sortie : SOE_pred (Joules ou unité cohérente avec la table)
// ============================================================================
float estimation_SOE(float SOC, float SOH, float moins_eta_sur_Q,
                     float *X_OCV, float *LOI_INTEG_OCV_DECHARGE, int n)
{
    // intégrale de courant prédite (Coulombs)
    float integrale_courant_pred = (1.0f / moins_eta_sur_Q) * SOH * SOC;

    // tension moyenne interpolée
    float ocv_moyenne = interp1rapide(X_OCV, LOI_INTEG_OCV_DECHARGE, n, SOC);

    // SOE (énergie)
    float SOE_pred = ocv_moyenne * integrale_courant_pred;

    return SOE_pred;
}

