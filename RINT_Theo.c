#include <stdio.h>
#include <stdlib.h>
#include "RINT_Theo.h"
#include "Read_Write.h"

// Garde-fou abs(float) sans <math.h>
static inline float f_absf(float x) { return (x < 0.0f) ? -x : x; }

void estimation_RINT_step(float tension,
                          float courant,
                          float SOC,
                          const float a_filtre_RINT[2],
                          const float b_filtre_RINT[2],
                          float *RINT,
                          float *SOHR,
                          float *RINT_INIT,
                          float *RINTkm1,
                          float *RINTfiltrekm1,
                          float *tension_precedente,
                          float *courant_precedent,
                          float *compteur_RINT,
                          float *RINT_ref)
{
    // --- Différentielles ---
    float delta_U = tension - *tension_precedente;
    float delta_I = courant - *courant_precedent;

    // --- Inhibition selon le script MATLAB ---
    int inhibition = (f_absf(delta_I) < 0.1f) || (SOC > 0.8f) || (SOC < 0.2f);

    // --- Estimation brute R = -ΔU/ΔI et filtrage IIR d'ordre 1 (si pas d'inhibition) ---
    float R = *RINT;  // valeur de repli si inhibition
    if (!inhibition) {
        if (delta_I != 0.0f) {
            R = -delta_U / delta_I;
        } else {
            // division par 0 -> on reste sur la valeur courante
            R = *RINT;
        }

        // Filtre : RINT_k = (-a2 * RINT(k-1) + b1 * R + b2 * R(k-1)) / a1
        float a1 = a_filtre_RINT[0];
        float a2 = a_filtre_RINT[1];
        float b1 = b_filtre_RINT[0];
        float b2 = b_filtre_RINT[1];

        if (a1 == 0.0f) {
            // garde-fou : si a1 nul, ne pas mettre à jour
        } else {
            *RINT = (-a2 * (*RINTfiltrekm1) + b1 * R + b2 * (*RINTkm1)) / a1;
            *RINTfiltrekm1 = *RINT;
            *RINTkm1       = R;
        }
    }

    // --- Phase d'attente/compteur + calcul SOHR ---
    if (*compteur_RINT < 250000.0f) {
        *compteur_RINT += 1.0f;
        *SOHR = 1.0f;
    } else if (!inhibition) {
        if (*RINT_INIT == -1.0f) {
            *RINT_INIT = *RINT;
            *SOHR = 1.0f;
        } else {
            // SOHR = 1 - (RINT - RINT_INIT)/RINT_INIT
            if (*RINT_INIT != 0.0f) {
                *SOHR = 1.0f - ((*RINT - *RINT_INIT) / (*RINT_INIT));
            } else {
                *SOHR = 1.0f; // garde-fou
            }
        }
    }

    // RINT_ref n'est pas utilisé dans le MATLAB fourni : on le laisse inchangé
    (void)RINT_ref;

    // --- MàJ des mémoires ---
    *tension_precedente = tension;
    *courant_precedent  = courant;
}

void setup(void)
{
    // === Entrées (buffers fournis par Charge_donnees) ===
    const float *courant     = NULL;
    const float *tension     = NULL;
    const float *temperature = NULL; // non utilisé ici
    const float *SOH         = NULL; // non utilisé ici
    const float *SOC_simu    = NULL;

    // Charge les données depuis l infrastructure commune
    size_t nb_samples = 1000000; // à adapter selon ta source
    Charge_donnees(&courant, &tension, &temperature, &SOH, &SOC_simu);

    // === Coefficients du filtre IIR d'ordre 1 (à remplacer par tes valeurs issues du .mat) ===
    // Exemple passe-bas symétrique (placeholders) — mets ici tes vrais a,b
    const float a_filtre_RINT[2] = { 1.0f, -0.95f };     // a1, a2
    const float b_filtre_RINT[2] = { 0.025f, 0.025f };   // b1, b2

    // === États internes (initialisation alignée sur le script) ===
    float RINT                = 0.018f;
    float RINT_INIT           = -1.0f;
    float RINTkm1             = 0.018f;
    float RINTfiltrekm1       = 0.018f;
    float tension_precedente  = 0.0f;
    float courant_precedent   = 1.0f;
    float compteur_RINT       = 0.0f;
    float RINT_ref            = -1.0f;
    float SOHR                = 1.0f;

    // === Boucle d estimation ===
    for (size_t i = 0; i < nb_samples; ++i) {
        float U   = tension[i];
        float I   = courant[i];
        float SOC = SOC_simu[i];

        estimation_RINT_step(U, I, SOC,
                             a_filtre_RINT, b_filtre_RINT,
                             &RINT, &SOHR, &RINT_INIT,
                             &RINTkm1, &RINTfiltrekm1,
                             &tension_precedente, &courant_precedent,
                             &compteur_RINT, &RINT_ref);

        // (optionnel) export/log, par ex. :
        // printf("%zu;%.6f;%.6f\n", i, RINT, SOHR);
    }

    // Libération des buffers
    Free_donnees(courant, tension, temperature, SOH, SOC_simu);
}

int main(void)
{
    setup();
    printf("Fin du programme\n");
    return 0;
}
