#ifndef RINT_H
#define RINT_H

// ============================================================================
// Contexte RINT : paramètres du filtre + états internes
// ============================================================================

typedef struct
{
    // Coefficients du filtre IIR d'ordre 1
    float a_filtre[2];   // a1, a2
    float b_filtre[2];   // b1, b2

    // États internes
    double RINT;          // résistance interne filtrée courante
    float  RINT_INIT;     // valeur de RINT au début de la vieillesse (ou -1)
    double RINTkm1;       // R(k-1) non filtré
    double RINTfiltrekm1; // RINT(k-1) filtré

    float tension_precedente;
    float courant_precedent;

    float compteur_RINT;  // phase d’attente avant calcul du SOHR
    float RINT_ref;       // non utilisé dans le MATLAB (laissé pour compatibilité)

    float SOHR;           // état de santé basé sur RINT (0–1)

} RINT_Context;

// ============================================================================
// Initialisation du contexte RINT
// ============================================================================
void RINT_init(RINT_Context *ctx);

//calcul "point par point" : met à jour le contexte
float RINT_step(RINT_Context *ctx,
                float tension,
                float courant,
                float SOC,
                float *SOHR_out);

#endif // RINT_THEO_H
