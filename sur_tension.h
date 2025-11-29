#ifndef SUR_TENSION_H
#define SUR_TENSION_H

// ============================================================================
// Fonction step "brute" : un échantillon → mise à jour Ir, U, alerte
// ============================================================================
//
// etat : 1 = décharge, 0 = charge
//
void surveillance_tension(float courant,
                          float SOC,
                          float tension_mesuree,
                          int   etat,
                          const float *X_OCV,
                          const float *Y_OCV_charge,
                          const float *Y_OCV_decharge,
                          int   n_OCV,
                          float dt,
                          float R1,
                          float C1,
                          float R0,
                          float seuil_alerte,
                          float *Ir,
                          float *U,
                          int   *alerte);

// ============================================================================
// Contexte pour utilisation simple en temps réel : TENSION_init + TENSION_step
// ============================================================================

typedef struct {
    // Paramètres électriques / modèle
    float dt;
    float R1;
    float C1;
    float R0;
    float seuil;

    // Tables OCV
    const float *X_OCV;
    const float *Y_OCV_charge;
    const float *Y_OCV_decharge;
    int   n_OCV;

    // État interne du filtre RC
    float Ir;
} TENSION_Context;

// Initialisation du contexte (paramètres + tables + état Ir)
void TENSION_init(TENSION_Context *ctx);

// Calcul "point par point"
// - entrée : courant, SOC, tension_mesuree, etat (0=charge, 1=décharge)
// - sortie : tension modèle U et alerte (0 ou 1)
// Retour : U (tension modèle)
float TENSION_step(TENSION_Context *ctx,
                   float courant,
                   float SOC,
                   float tension_mesuree,
                   int   etat,
                   int  *alerte);

#endif // SURVEILLANCE_TENSION_H
