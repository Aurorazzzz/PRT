#ifndef SURVEILLANCE_TENSION_H
#define SURVEILLANCE_TENSION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Surveillance de la tension cellule
 *
 * Entrées :
 *   courant         : courant instantané (A)
 *   SOC             : état de charge [0..1]
 *   tension_mesuree : tension mesurée (V)
 *   etat            : 1 = décharge, 0 = charge
 *   X_OCV           : grille SOC pour les lois OCV        (taille n_OCV)
 *   Y_OCV_charge    : loi OCV en charge                   (taille n_OCV)
 *   Y_OCV_decharge  : loi OCV en décharge                 (taille n_OCV)
 *   n_OCV           : taille des tableaux OCV
 *   dt              : pas de temps (s)
 *   R1, C1          : paramètres du réseau RC
 *   R0              : résistance série
 *   seuil_alerte    : seuil sur |tension_mesuree - U_modele|
 *
 * In/Out :
 *   Ir              : état interne du modèle RC (courant dans la branche R1-C1)
 *
 * Sorties :
 *   U               : tension modèle calculée
 *   alerte          : 0 ou 1 suivant dépassement du seuil
 */
void surveillance_tension(float courant,
                          float SOC,
                          float tension_mesuree,
                          int   etat,
                          const float *X_OCV,
                          const float *Y_OCV_charge,
                          const float *Y_OCV_decharge,
                          int   n_OCV,
                          float dt,
                          float R1,
                          float C1,
                          float R0,
                          float seuil_alerte,
                          float *Ir,
                          float *U,
                          int   *alerte);

/* Fonction d’initialisation / boucle principale, comme dans SOE_Theo.c */
void setup(void);

#ifdef __cplusplus
}
#endif

#endif /* SURVEILLANCE_TENSION_H */
