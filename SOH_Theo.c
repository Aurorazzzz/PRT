#include <stdio.h>
#include <stdlib.h>
#include "SOH_Theo.h"
#include "Read_Write.h"
#include <stddef.h>
#include <math.h>

// ============================================================================
// Outils
// ============================================================================
static inline float f_abs(float x) { return (x < 0.0f) ? -x : x; }

// --------------------------------------------------------------------------
// Traduction C de detection_phase_charge_decharge.m
// tampon_charge_decharge : tableau de taille 'taille_tampon'
// etat / etat_precedent  : 0 = charge, 1 = décharge
// --------------------------------------------------------------------------
static void detection_phase_charge_decharge(float courant,
                                            float *tampon_charge_decharge,
                                            int   taille_tampon,
                                            int  *etat_precedent,
                                            int  *etat,
                                            int  *changement_etat)
{
    // Décalage du tampon : elem[k] = elem[k-1]
    for (int i = taille_tampon - 1; i > 0; --i) {
        tampon_charge_decharge[i] = tampon_charge_decharge[i - 1];
    }
    tampon_charge_decharge[0] = courant;

    // Moyenne du tampon
    float somme = 0.0f;
    for (int i = 0; i < taille_tampon; ++i) {
        somme += tampon_charge_decharge[i];
    }
    float moyenne_charge_decharge = somme / (float)taille_tampon;

    // Mise à jour de l'état charge / décharge
    // état = 0 --> charge
    // état = 1 --> décharge
    if (moyenne_charge_decharge > 0.1f && !(*etat_precedent)) {
        // on était en charge, on passe en décharge
        *etat = 1;
    } else if (moyenne_charge_decharge < -1.0f && *etat_precedent) {
        // on était en décharge, on passe en charge
        *etat = 0;
    } else {
        *etat = *etat_precedent;
    }

    // Détection du changement d'état
    if (*etat == *etat_precedent) {
        *changement_etat = 0;
    } else {
        *changement_etat = 1;
    }

    *etat_precedent = *etat;
}

// ============================================================================
// Fonction principale : estimation du SOH (traduction C de estimation_SOH.m)
// ============================================================================
float estimation_SOH(float SOC,
                     float courant,
                     float dt,
                     int   changement_etat,
                     float integrale_courant_neuf,
                     const float a_filtre_SOH[2],
                     const float b_filtre_SOH[2],
                     float *integrale_courant,
                     float *SOC_eval_SOH_precedent,
                     float *SOH_filtre_km1,
                     float *SOH_depouille_km1,
                     float SOH_courant)
{
    // Comptage du courant
    *integrale_courant += courant * dt;

    if (changement_etat)
    {
        // condition : au moins 10 % de la pleine charge à neuf
        int condition_SOH =
            (f_abs(*integrale_courant) / integrale_courant_neuf > 0.1f);

        if (condition_SOH)
        {
            // SOH_depouille = intégrale_courant /
            //                 ((SOC_eval_SOH_precedent - SOC) * integrale_courant_neuf)
            float delta_SOC = (*SOC_eval_SOH_precedent - SOC);
            float denom = delta_SOC * integrale_courant_neuf;
            float SOH_depouille =
                (denom != 0.0f) ? (*integrale_courant) / denom : SOH_courant;

            // saturation pour éviter les valeurs incohérentes
            if (SOH_depouille > 1.0f || SOH_depouille < 0.0f)
                SOH_depouille = SOH_courant;

            // Filtre d'ordre 1
            float SOH_new = (-a_filtre_SOH[1] * (*SOH_filtre_km1)
                             + b_filtre_SOH[0] * SOH_depouille
                             + b_filtre_SOH[1] * (*SOH_depouille_km1))
                            / a_filtre_SOH[0];

            *SOH_filtre_km1    = SOH_new;
            *SOH_depouille_km1 = SOH_depouille;
            SOH_courant        = SOH_new;
        }

        *SOC_eval_SOH_precedent = SOC;
        *integrale_courant = 0.0f;
    }

    return SOH_courant;
}

// ============================================================================
// Programme principal
// ============================================================================
void setup(void)
{
    // === Entrées ===
    const float *courant     = NULL;
    const float *tension     = NULL;
    const float *temperature = NULL;
    const float *SOH_ref     = NULL;
    const float *SOC_simu    = NULL;

    // Chargement des données
    size_t nb_samples = 1000000;
    Charge_donnees(&courant, &tension, &temperature, &SOH_ref, &SOC_simu);

    // === Paramètres du filtre SOH ===
    const float a_filtre_SOH[2] = { 1.0f, -0.969067417193793f };
    const float b_filtre_SOH[2] = { 0.0154662914031034f, 0.0154662914031034f };

    const float dt = 1.0f;

    // === Comptage coulombmétrique : même valeur que dans script_principal.m ===
    const float moins_eta_sur_Q         = 2.300303904920101e-04f;
    const float integrale_courant_neuf  = 1.0f / moins_eta_sur_Q;

    // === États internes SOH ===
    float integrale_courant      = 0.0f;
    float SOC_eval_SOH_precedent = SOC_simu[0];
    float SOH_filtre_km1         = 1.0f;
    float SOH_depouille_km1      = 1.0f;
    float SOH_courant            = 1.0f;

    // === États pour la détection charge/décharge ===
    #define TAILLE_TAMPON 60
    float tampon_charge_decharge[TAILLE_TAMPON] = {0.0f};
    int   etat_precedent = 0;   // 0 : charge, 1 : décharge
    int   etat           = 0;
    int   changement_etat = 0;

    // === Buffers de sortie ===
    float *SOH_result = malloc(nb_samples * sizeof(float));
    if (!SOH_result)
    {
        perror("Erreur allocation mémoire");
        return;
    }

    // === Calcul du SOH ===
    for (size_t i = 0; i < nb_samples; ++i)
    {
        float SOC_i = SOC_simu[i];
        float I_i   = courant[i];

        // Détection de la phase charge/décharge (comme MATLAB)
        detection_phase_charge_decharge(
            I_i,
            tampon_charge_decharge,
            TAILLE_TAMPON,
            &etat_precedent,
            &etat,
            &changement_etat
        );

        float SOH_est = estimation_SOH(
            SOC_i,
            I_i,
            dt,
            changement_etat,
            integrale_courant_neuf,
            a_filtre_SOH,
            b_filtre_SOH,
            &integrale_courant,
            &SOC_eval_SOH_precedent,
            &SOH_filtre_km1,
            &SOH_depouille_km1,
            SOH_courant
        );

        SOH_courant   = SOH_est;
        SOH_result[i] = SOH_est;
    }

    // === Sauvegarde du résultat ===
    Ecriture_result(SOH_result, nb_samples, "SOH_MODEL_raspi_result2");

    // === Libération mémoire ===
    Free_donnees(courant, tension, temperature, SOH_ref, SOC_simu);
    free(SOH_result);
}

int main(void)
{
    setup();
    printf("Fin du programme\n");
    return 0;
}
