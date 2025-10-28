/* estimation_SOH.h
 * Prototype de la fonction d'estimation du SOH (C simple précision).
 * Aucune dépendance externe ; compatible C et C++.
 *
 * Auteur : (vous)
 * Licence : à définir
 */

#ifndef ESTIMATION_SOH_H
#define ESTIMATION_SOH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estime et filtre le SOH à la détection d'un changement d'état (charge↔décharge).
 *
 * Forme du filtre IIR d'ordre 1 utilisée :
 *   SOH(k) = (-a2 * SOH(k-1) + b1 * x(k) + b2 * x(k-1)) / a1
 * où x(k) est le SOH « dépouillé » (non filtré) estimé à l'instant k.
 *
 * @param SOC                         État de charge actuel [0..1].
 * @param courant                     Courant instantané (A).
 * @param dt                          Pas de temps (s).
 * @param changement_etat             0/1 : front de bascule charge↔décharge.
 * @param integrale_courant_neuf      Intégrale de courant à neuf (A·s) ≈ capacité utile/η.
 * @param a_filtre_SOH                Coefficients a[2] du filtre (a1, a2).
 * @param b_filtre_SOH                Coefficients b[2] du filtre (b1, b2).
 * @param integrale_courant           [in/out] Intégrale ∫I·dt accumulée depuis la dernière bascule (A·s).
 * @param SOC_eval_SOH_precedent      [in/out] SOC mémorisé lors de la précédente bascule.
 * @param SOH_filtre_km1              [in/out] Dernière valeur filtrée SOH(k-1).
 * @param SOH_depouille_km1           [in/out] Dernière valeur dépouillée x(k-1).
 * @param SOH_courant                 Valeur de repli/initiale du SOH si pas de mise à jour.
 *
 * @return SOH courant (nouveau SOH filtré si mise à jour, sinon SOH_courant).
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

#endif /* ESTIMATION_SOH_H */
