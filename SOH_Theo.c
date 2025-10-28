#include <stdio.h>
#include <stdlib.h>
#include "SOH_Theo.h"
#include "Read_Write.h"
#include <stddef.h>

// ============================================================================
// Estimation du SOH (traduction C du script MATLAB)
// Entrées :
//   SOC                         : état de charge actuel [0..1]
//   courant                     : courant instantané (A)
//   dt                          : pas de temps (s)
//   changement_etat             : 0/1 (bascule charge<->décharge détectée)
//   integrale_courant_neuf      : intégrale de courant à neuf (A·s) = Q_neuf / η
//   a_filtre_SOH[2], b_filtre_SOH[2] : coeffs du filtre IIR d'ordre 1
//   *integrale_courant          : intégrale depuis la dernière estimation (A·s) [in/out]
//   *SOC_eval_SOH_precedent     : SOC mémorisé au dernier calcul de SOH [in/out]
//   *SOH_filtre_km1             : SOH filtré(k-1) [in/out]
//   *SOH_depouille_km1          : SOH dépouillé(k-1) [in/out]
//   SOH_courant                 : SOH courant (valeur de repli / saturation) [in]
// Sortie :
//   SOH filtré mis à jour (si estimation effectuée), sinon SOH_courant.
// Remarques :
//   - Comptage coulombmétrique cumulé entre deux changements d’état.
//   - Estimation déclenchée si |∫I dt| > 10% de la pleine charge à neuf.
//   - « Règle de trois » puis filtrage IIR d’ordre 1.
//   - Saturation [0,1] sur la valeur dépouillée (sinon on garde SOH_courant).
// ============================================================================
static inline float f_abs(float x) { return (x < 0.0f) ? -x : x; }

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
                     float SOH_courant){
                        
    // Comptage du courant
    *integrale_courant += courant * dt;

    if (changement_etat) {
        // Condition d'activation (>10% de la pleine charge à neuf)
        int condition_SOH = (f_abs(*integrale_courant) / integrale_courant_neuf > 0.1f);

        if (condition_SOH) {
            // Estimation « règle de trois »
            float delta_SOC = (*SOC_eval_SOH_precedent - SOC);
            float denom = delta_SOC * integrale_courant_neuf;
            float SOH_depouille = (denom != 0.0f) ? (*integrale_courant) / denom : SOH_courant;

            // Saturation / garde-fou
            if (SOH_depouille > 1.0f || SOH_depouille < 0.0f) {
                SOH_depouille = SOH_courant;
            }

            // Filtre IIR (ordre 1)
            float SOH_new = (-a_filtre_SOH[1] * (*SOH_filtre_km1)
                             + b_filtre_SOH[0] * SOH_depouille
                             + b_filtre_SOH[1] * (*SOH_depouille_km1))
                            / a_filtre_SOH[0];

            // Mise à jour des états
            *SOH_filtre_km1    = SOH_new;
            *SOH_depouille_km1 = SOH_depouille;
            SOH_courant        = SOH_new;
        }

        // Mémorisation & RAZ intégrale
        *SOC_eval_SOH_precedent = SOC;
        *integrale_courant = 0.0f;
    }

    return SOH_courant;
}

void setup(void)
{
    // === Entrées (buffers fournis par Charge_donnees) ===
    const float *courant    = NULL;
    const float *tension    = NULL;
    const float *temperature= NULL;
    const float *SOH        = NULL;
    const float *SOC_simu   = NULL;

    // Charge les données
    size_t nb_samples = 1000000;
    Charge_donnees(&courant, &tension, &temperature, &SOH, &SOC_simu);

    // === Paramètres ===
    // Coeffs du filtre IIR d'ordre 1 (a1, a2) et (b1, b2)
    const float a_filtre_SOH[2] = { 1.0f, -0.969067417193793f };
    const float b_filtre_SOH[2] = { 0.0154662914031034f, 0.0154662914031034f };

    // Pas d’échantillonnage
    // à remplacer par la vraie valeur (p.ex. fournie par les données)
    const float dt = 1.0f;

    // Intégrale de courant à neuf (A·s) = Q_neuf (A·h) * 3600 / eta
    // TODO: mets ici tes vraies valeurs (ex: 50 Ah, eta=0.99 => 50*3600/0.99)
    const float capacite_Ah_neuf = 50.0f;     // TODO
    const float eta_coulombique  = 0.99f;     // TODO
    const float integrale_courant_neuf =
        (capacite_Ah_neuf * 3600.0f) / eta_coulombique;

    // === États internes (initialisation) ===
    float integrale_courant      = 0.0f;
    float SOC_eval_SOH_precedent = (nb_samples > 0) ? SOC_simu[0] : 0.0f;
    float SOH_filtre_km1         = (SOH ? SOH[0] : 1.0f);
    float SOH_depouille_km1      = SOH_filtre_km1;
    float SOH_courant            = SOH_filtre_km1;

    // === Calcul du SOH pour chaque échantillon ===
    for (size_t i = 0; i < nb_samples; ++i) {
        float SOC_i     = SOC_simu[i];   // SOC à l’instant i
        float I_i       = courant[i];    // courant à l’instant i (A)

        // Détection simple d'une bascule charge↔décharge par changement de signe du courant
        int changement_etat = 0;
        if (i > 0) {
            float I_prev = courant[i - 1];
            if ( (I_prev >= 0.0f && I_i < 0.0f) || (I_prev <= 0.0f && I_i > 0.0f) ) {
                changement_etat = 1;
            }
        }

        // Estimation / mise à jour du SOH
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
                            SOH_courant);

        // La valeur courante devient la sortie retournée
        SOH_courant = SOH_est;

        // Affichage (adapte selon ton besoin : fichier, log, etc.)
        printf("i=%zu  SOC=%.6f  SOH_estime=%.6f\n", i, SOC_i, SOH_est);
    }

    // Libération des buffers
    Free_donnees(courant, tension, temperature, SOH, SOC_simu);
}


int main() {
    setup();
    printf("Fin du programme\n");
    return 0;
}
