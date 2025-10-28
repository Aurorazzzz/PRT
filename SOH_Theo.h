/* SOH_Theo.h
 * Prototype de la fonction d'estimation/filtrage du SOH.
 * Compatible C/C++.
 */

#ifndef SOH_THEO_H
#define SOH_THEO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estime et filtre le SOH lors d’un changement d’état (charge↔décharge).
 *
 * Filtre IIR d’ordre 1 (forme directe I) :
 *   SOH(k) = (-a2 * SOH(k-1) + b1 * x(k) + b2 * x(k-1)) / a1,
 * avec x(k) = SOH dépouillé à l’instant k.
 *
 * @param SOC                         État de charge actuel [0..1].
 * @param courant                     Courant instantané (A).
 * @param dt                          Pas de temps (s).
 * @param changement_etat             0/1 : front de bascule charge↔décharge.
 * @param integrale_courant_neuf      ∫I·dt à neuf (A·s) ≈ (Q_neuf/η).
 * @param a_filtre_SOH                Coefficients a[2] = {a1, a2}.
 * @param b_filtre_SOH                Coefficients b[2] = {b1, b2}.
 * @param integrale_courant           [in/out] ∫I·dt accumulé depuis la dernière bascule (A·s).
 * @param SOC_eval_SOH_precedent      [in/out] SOC mémorisé à la précédente bascule.
 * @param SOH_filtre_km1              [in/out] Dernière sortie filtrée SOH(k-1).
 * @param SOH_depouille_km1           [in/out] Dernier SOH dépouillé x(k-1).
 * @param SOH_courant                 Valeur de repli/initiale si pas de mise à jour.
 *
 * @return SOH courant (nouvelle valeur filtrée si mise à jour, sinon SOH_courant).
 */
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
                     float SOH_courant);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SOH_THEO_H */
