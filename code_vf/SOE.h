#ifndef SOE_THEO_H
#define SOE_THEO_H

// Fonction pure (déjà existante)
float estimation_SOE(float SOC, float SOH, float moins_eta_sur_Q,
                     const float *X_OCV, const float *LOI_INTEG_OCV_DECHARGE, int n);

// Petit contexte pour éviter de repasser les tables partout
typedef struct {
    float moins_eta_sur_Q;
    const float *X_OCV;
    const float *LOI_INTEG_OCV_DECHARGE;
    int n;
} SOE_Context;

// Initialisation du contexte (tables + constante)
void SOE_init(SOE_Context *ctx);

// Calcul "point par point"
float SOE_step(float SOC, float SOH, const SOE_Context *ctx);

#endif
