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
                     float SOH_courant)
{
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
