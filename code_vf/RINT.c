#include "RINT.h"

// Garde-fou abs(float) sans <math.h>
static inline float f_absf(float x) { return (x < 0.0f) ? -x : x; }

// ============================================================================
// Noyau de calcul : traduction de votre estimation_RINT_step()
// ============================================================================

static void estimation_RINT_step_core(float tension,
                                      float courant,
                                      float SOC,
                                      RINT_Context *ctx)
{
    if (!ctx) return;

    // --- Différentielles ---
    float delta_U = tension - ctx->tension_precedente;
    float delta_I = courant - ctx->courant_precedent;

    // --- Inhibition selon le script MATLAB ---
    int inhibition = (f_absf(delta_I) < 0.1f) || (SOC > 0.8f) || (SOC < 0.2f);

    // --- Estimation brute R = -ΔU/ΔI et filtrage IIR d'ordre 1 (si pas d'inhibition) ---
    double R = ctx->RINT;  // valeur de repli si inhibition

    if (!inhibition)
    {
        if (delta_I != 0.0f) {
            R = -delta_U / delta_I;
        }

        float a1 = ctx->a_filtre[0];
        float a2 = ctx->a_filtre[1];
        float b1 = ctx->b_filtre[0];
        float b2 = ctx->b_filtre[1];

        ctx->RINT = (-a2 * (ctx->RINTfiltrekm1)
                     + b1 * R
                     + b2 * (ctx->RINTkm1)) / a1;

        ctx->RINTfiltrekm1 = ctx->RINT;
        ctx->RINTkm1       = R;
    }

    // --- Phase d'attente/compteur + calcul SOHR ---
    if (ctx->compteur_RINT < 250000.0f) {
        ctx->compteur_RINT += 1.0f;
        ctx->SOHR = 1.0f;
    }
    else if (!inhibition)
    {
        if (ctx->RINT_INIT == -1.0f) {
            ctx->RINT_INIT = (float)ctx->RINT;
            ctx->SOHR      = 1.0f;
        } else {
            if (ctx->RINT_INIT != 0.0f) {
                ctx->SOHR = 1.0f - ((float)(ctx->RINT - ctx->RINT_INIT) / ctx->RINT_INIT);
            } else {
                ctx->SOHR = 1.0f; // garde-fou
            }
        }
    }

    // RINT_ref n'est pas utilisé dans le MATLAB fourni : on le laisse inchangé
    (void)ctx->RINT_ref;

    // --- MàJ des mémoires ---
    ctx->tension_precedente = tension;
    ctx->courant_precedent  = courant;
}

// ============================================================================
// Initialisation du contexte
// ============================================================================

void RINT_init(RINT_Context *ctx)
{
    if (!ctx) return;

    // Coefficients du filtre IIR d'ordre 1 (vos valeurs d’origine)
    ctx->a_filtre[0] = 1.0f;
    ctx->a_filtre[1] = -0.999968584566934f;

    ctx->b_filtre[0] = 0.00001570771653f;
    ctx->b_filtre[1] = 0.00001570771650f;

    // États internes (initialisation alignée sur votre code initial)
    ctx->RINT          = 0.018;    // double
    ctx->RINT_INIT     = -1.0f;
    ctx->RINTkm1       = 0.018;
    ctx->RINTfiltrekm1 = 0.018;

    ctx->tension_precedente = 0.0f;
    ctx->courant_precedent  = 1.0f;
    ctx->compteur_RINT      = 0.0f;
    ctx->RINT_ref           = -1.0f;

    ctx->SOHR               = 1.0f;
}

// ============================================================================
// Step public : un échantillon (tension, courant, SOC)
// ============================================================================

float RINT_step(RINT_Context *ctx,
                float tension,
                float courant,
                float SOC,
                float *SOHR_out)
{
    if (!ctx) return 0.0f;

    // Ici, à vous de décider si vous passez I ou -I à la fonction.
    // Dans votre code initial, vous appeliez estimation_RINT_step(U, -I, SOC, ...).
    // Si vous voulez garder EXACTEMENT la même convention :
    float courant_effectif = courant; // ou -courant si nécessaire côté appelant

    estimation_RINT_step_core(tension, courant_effectif, SOC, ctx);

    if (SOHR_out) {
        *SOHR_out = ctx->SOHR;
    }

    // ctx->RINT est un double, on renvoie un float
    return (float)ctx->RINT;
}
