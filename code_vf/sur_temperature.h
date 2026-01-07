#ifndef SUR_TEMPERATURE_H
#define SUR_TEMPERATURE_H

// ============================================================================
// Fonction step "brute" : un échantillon → mise à jour T1/T2 + alerte
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
// Contexte pour utilisation simple en temps réel : TEMP_init + TEMP_step
// ============================================================================

typedef struct {
    // Paramètres du modèle thermique
    float R1;
    float C1;
    float R2;
    float C2;

    float seuil_alerte_temperature;
    float TAMB;
    float dt;

    // États internes du modèle
    float T1;
    float T2;
} TEMP_Context;

// Initialisation du contexte (paramètres + états initiaux)
void TEMP_init(TEMP_Context *ctx);

// Calcul "point par point" : met à jour le contexte
// - entrée : courant, temperature
// - sortie : T2 (température modèle) et alerte (0 ou 1)
float TEMP_step(TEMP_Context *ctx,
                float courant,
                float temperature,
                int  *alerte);

#endif // SURVEILLANCE_TEMPERATURE_H
