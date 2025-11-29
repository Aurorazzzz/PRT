#include <math.h>
#include <string.h>
#include "SOH.h"

// ============================================================================
// Fonctions internes (static) : moyenne, détection, calcul SOH
// ============================================================================

static float moyenne(const float *tableau, int taille)
{
    float somme = 0.0f;
    for (int i = 0; i < taille; i++) {
        somme += tableau[i];
    }
    return somme / (float)taille;
}

// Détection charge/décharge : met à jour ctx->etat_precedent et renvoie changement_etat
static bool detection_charge_decharge_core(SOH_Context *ctx,
                                           float courant)
{
    // Décalage du tampon vers la droite
    memmove(&ctx->tampon_charge_decharge[1],
            &ctx->tampon_charge_decharge[0],
            (ctx->taille_tampon - 1) * sizeof(float));

    ctx->tampon_charge_decharge[0] = courant;

    float moyenne_charge_decharge = moyenne(ctx->tampon_charge_decharge,
                                            ctx->taille_tampon);

    bool etat;
    // Mise à jour de l'état charge/décharge
    if ((moyenne_charge_decharge > 0.1f) && !ctx->etat_precedent) {
        etat = true;   // passe en décharge
    } else if ((moyenne_charge_decharge < -1.0f) && ctx->etat_precedent) {
        etat = false;  // passe en charge
    } else {
        etat = ctx->etat_precedent; // pas de changement
    }

    bool changement_etat = (etat != ctx->etat_precedent);
    ctx->etat_precedent  = etat;

    return changement_etat;
}

// Calcul du SOH (noyau de calcul) — traduction directe de calcul_SOH()
static void calcul_SOH_core(SOH_Context *ctx,
                            float courant,
                            bool  changement_etat,
                            float SOC)
{
    // Intégration du courant
    ctx->integrale_courant += courant * ctx->dt;

    if (changement_etat)
    {
        // condition : l'intégrale de courant parcourue depuis la dernière
        // estimation représente au moins 10% de la pleine charge à neuf
        float ratio = fabsf(ctx->integrale_courant) / ctx->integrale_courant_neuf;
        bool condition_SOH = (ratio > 0.1f);

        if (condition_SOH) {
            // Estimation par règle de trois :
            // SOH_pre_filtre = (intégrale courant) /
            //                  ((écart SOC) * intégrale_courant_neuf)
            float delta_SOC = ctx->SOC_precedent - SOC;
            float denom     = delta_SOC * ctx->integrale_courant_neuf;

            if (denom != 0.0f) {
                float SOH_pre_filtre = ctx->integrale_courant / denom;

                // Saturation pour éviter les valeurs aberrantes
                if ((SOH_pre_filtre > 1.0f) || (SOH_pre_filtre < 0.0f)) {
                    SOH_pre_filtre = ctx->SOH;
                }

                // Filtrage du SOH : filtre discret d'ordre 1
                // y(n) = (-a1*y(n-1) + b0*x(n) + b1*x(n-1)) / a0
                ctx->SOH = (float)(
                    (-ctx->a_filtre[1] * ctx->y_n_1
                     + ctx->b_filtre[0] * SOH_pre_filtre
                     + ctx->b_filtre[1] * ctx->x_n_1) / ctx->a_filtre[0]
                );

                ctx->y_n_1 = ctx->SOH;
                ctx->x_n_1 = SOH_pre_filtre;
            }
        }

        // Mise à jour du SOC de référence et remise à zéro de l'intégrale
        ctx->SOC_precedent    = SOC;
        ctx->integrale_courant = 0.0f;
    }
}

// ============================================================================
// Initialisation du contexte
// ============================================================================

void SOH_init(SOH_Context *ctx)
{
    if (!ctx) return;

    ctx->taille_tampon = 60;
    ctx->dt            = 1.0f;
    ctx->moins_eta_sur_Q = 0.00023003f;
    ctx->integrale_courant_neuf = 1.0f / ctx->moins_eta_sur_Q;

    // Coefficients du filtre
    ctx->a_filtre[0] = 1.0;
    ctx->a_filtre[1] = -0.969067417193793;
    ctx->b_filtre[0] = 0.0154662914031034;
    ctx->b_filtre[1] = 0.0154662914031034;

    // Tampon courant
    for (int i = 0; i < ctx->taille_tampon; ++i) {
        ctx->tampon_charge_decharge[i] = 0.0f;
    }
    ctx->etat_precedent   = false;

    // Variables SOH
    ctx->integrale_courant = 0.0f;
    ctx->SOC_precedent     = 0.0f;

    // Conditions initiales du filtre / SOH
    ctx->SOH    = 1.0f;
    ctx->y_n_1  = 1.0f;
    ctx->x_n_1  = 1.0f;
}

// ============================================================================
// Step SOH : un échantillon → mise à jour + SOH courant
// ============================================================================

float SOH_step(SOH_Context *ctx, float courant, float SOC)
{
    if (!ctx) return 1.0f;

    // même convention que SOH_setup : on utilise -courant pour la détection et le calcul
    float courant_sim = -courant;

    // Détection charge/décharge
    bool changement_etat = detection_charge_decharge_core(ctx, courant_sim);

    // Calcul du SOH
    calcul_SOH_core(ctx, courant_sim, changement_etat, SOC);

    // Retourne la valeur actuelle du SOH filtré
    return ctx->SOH;
}
