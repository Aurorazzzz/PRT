#ifndef SOH_V1_H
#define SOH_V1_H

#include <stdbool.h>

// ============================================================================
// Contexte SOH : paramètres + états internes
// ============================================================================

typedef struct
{
    // Paramètres généraux
    int   taille_tampon;          // typiquement 60
    float dt;                     // pas de temps (s)
    float moins_eta_sur_Q;        // 1 / (eta * Q)
    float integrale_courant_neuf; // 1 / moins_eta_sur_Q

    // Coefficients du filtre (ordre 1)
    double a_filtre[2];
    double b_filtre[2];

    // Tampon pour détection charge/décharge
    float tampon_charge_decharge[60]; // taille max 60
    bool  etat_precedent;

    // Variables pour le SOH
    float integrale_courant;
    float SOC_precedent;
    float SOH;       // valeur filtrée actuelle
    float y_n_1;     // sortie filtrée à l’instant n-1
    float x_n_1;     // entrée filtre à l’instant n-1 (SOH_pre_filtre_n_1)

} SOH_Context;

// ============================================================================
// Initialisation du contexte (paramètres + états initiaux)
// ============================================================================
void SOH_init(SOH_Context *ctx);

// ============================================================================
// Step SOH : un échantillon (courant, SOC) → mise à jour du contexte + SOH
// - courant : courant batterie (même convention que votre MATLAB : on appliquera
//             le signe à l’intérieur comme dans SOH_setup : -courant)
// - SOC     : SOC instantané (0–1)
// Retour    : SOH(t), valeur filtrée actuelle
// ============================================================================
float SOH_step(SOH_Context *ctx, float courant, float SOC);

#endif // SOH_V1_H
