#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "surveillance_temperature.h"
#include "Read_Write.h"

// ============================================================================
// Fonction principale : surveillance température (équivalent MATLAB)
// Entrées :
//   courant, temperature       : mesures instantanées
//   *T1, *T2                   : états thermiques internes (in/out)
//   R1, C1, R2, C2             : paramètres du modèle thermique
//   seuil_alerte_temperature   : seuil absolu entre Tmesurée et T2
//   TAMB                       : température ambiante
//   dt                         : pas de temps
// Sorties :
//   *T1, *T2                   : états internes mis à jour
//   *alerte                    : 1 si |temperature - T2| > seuil
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
                      TAMB - *T1) / (R1_modele_thermique * C1_modele_thermique);

    *T2 = *T2 + dt * (*T1 - *T2) / (R2_modele_thermique * C2_modele_thermique);

    // Détection d'écart température mesurée vs modèle
    float diff = fabsf(temperature - *T2);
    *alerte = (diff > seuil_alerte_temperature) ? 1 : 0;
}

// ============================================================================
// Programme autonome — similaire aux fichiers SOE_Theo / SOH_Theo
// ============================================================================
void setup(void)
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

    // Paramètres du modèle thermique (à adapter selon les données MATLAB)
    float R1 = 0.5f;
    float C1 = 1500.0f;
    float R2 = 0.7f;
    float C2 = 2500.0f;
    float seuil = 10.0f;
    float TAMB = 25.0f;
    float dt = 1.0f;

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
            R1, C1,
            R2, C2,
            seuil,
            TAMB,
            dt,
            &alerte
        );

        vecteur_T2[i] = T2;
        vecteur_alerte[i] = (float)alerte;
    }

    // Sauvegarde
    Ecriture_result(vecteur_T2, NbIteration, "TEMPERATURE_MODEL_raspi_result");
    Ecriture_result(vecteur_alerte, NbIteration, "ALERTE_TEMPERATURE_raspi_result");

    Free_donnees(courant, tension, temperature, SOH, SOC_simu);

    free(vecteur_T2);
    free(vecteur_alerte);
}

int main()
{
    setup();
    printf("Fin du programme !\n");
    return 0;
}
