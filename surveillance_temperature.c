#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "surveillance_temperature.h"
#include "Read_Write.h"

// ============================================================================
// Fonction principale : surveillance température (équivalent MATLAB)
// ============================================================================
void surveillance_temperature(float courant,
                              float temperature,
                              float *T1,
                              float *T2,
                              float R1_modele_thermique,
                              float C1_modele_thermique,
                              float R2_modele_thermique,
                              float C2_modele_thermique,
                              float seuil_alerte_temperature,
                              float TAMB,
                              float dt,
                              int *alerte)
{
    // Modèle thermique Foster d'ordre 2
    *T1 = *T1 + dt * (R1_modele_thermique * courant * courant +
                      TAMB - *T1) /
                      (R1_modele_thermique * C1_modele_thermique);

    *T2 = *T2 + dt * (*T1 - *T2) /
                      (R2_modele_thermique * C2_modele_thermique);

    // Détection d'écart température mesurée vs modèle
    float diff = fabsf(temperature - *T2);
    *alerte = (diff > seuil_alerte_temperature) ? 1 : 0;
}

// ============================================================================
// Programme autonome — similaire aux fichiers SOE_Theo / SOH_Theo
// ============================================================================
void TEMP_setup(void)
{
    const float *courant = NULL;
    const float *tension = NULL;
    const float *temperature = NULL;
    const float *SOH = NULL;
    const float *SOC_simu = NULL;

    const int NbIteration = 1000000;

    float *vecteur_T2 = malloc(NbIteration * sizeof(float));
    float *vecteur_alerte = malloc(NbIteration * sizeof(float));

    if (!vecteur_T2 || !vecteur_alerte) {
        perror("Erreur allocation mémoire");
        return;
    }

    // =====================================================================
    // Paramètres EXACTS du modèle thermique MATLAB
    // =====================================================================
    const float R1_modele_thermique = 0.206124119186158f;
    const float C1_modele_thermique = 50.3138901982787f;
    const float R2_modele_thermique = 21.6224372540937f;
    const float C2_modele_thermique = 15.8943772584241f;

    const float seuil = 10.0f;   // conforme MATLAB
    const float TAMB  = 25.0f;   // conforme MATLAB
    const float dt    = 1.0f;    // conforme MATLAB

    // Conditions initiales MATLAB
    float T1 = 60.0f;
    float T2 = 30.0f;

    int alerte = 0;

    // Chargement des données
    Charge_donnees(&courant, &tension, &temperature, &SOH, &SOC_simu);

    for (int i = 0; i < NbIteration; ++i)
    {
        surveillance_temperature(
            courant[i],
            temperature[i],
            &T1,
            &T2,
            R1_modele_thermique, C1_modele_thermique,
            R2_modele_thermique, C2_modele_thermique,
            seuil,
            TAMB,
            dt,
            &alerte
        );

        vecteur_T2[i] = T2;
        vecteur_alerte[i] = (float)alerte;
    }

    // Sauvegarde
    Ecriture_result(vecteur_T2, NbIteration, "TEMPERATURE_MODEL_PC_result");
    Ecriture_result(vecteur_alerte, NbIteration, "ALERTE_TEMPERATURE_PC_result");


    Free_donnees(courant, tension, temperature, SOH, SOC_simu);

    free(vecteur_T2);
    free(vecteur_alerte);

    //printf("R1 = %.12f, C1 = %.12f\n", R1_modele_thermique, C1_modele_thermique);
    //printf("R2 = %.12f, C2 = %.12f\n", R2_modele_thermique, C2_modele_thermique);

}
/*
int main(void)
{
    TEMP_setup();
    printf("Fin du programme !\n");
    return 0;
}*/
