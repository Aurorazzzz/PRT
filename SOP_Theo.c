#include <stdio.h>
#include <stdlib.h>
#include "SOP_Theo.h"
#include "Read_Write.h"
#include <stddef.h>

// ============================================================================
// Estimation du SOP (traduction C du script MATLAB)
// Entrées :
//  
// Sortie :
//   
// Remarques :
//   
// ============================================================================

void estimation_SOP(float inputArg1,
                    float inputArg2,
                    float* outputArg1,
                    float* outputArg2)
{
    if (outputArg1) *outputArg1 = inputArg1;
    if (outputArg2) *outputArg2 = inputArg2;
}

void SOP_setup(void)
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

    // === Calcul du SOP pour chaque échantillon ===
    for (size_t i = 0; i < nb_samples; ++i) {
        float in1  = courant[i];
        float in2  = tension[i];
        float out1 = 0.0f;
        float out2 = 0.0f;

        estimation_SOP(in1, in2, &out1, &out2);
    }

    // Libération des buffers
    Free_donnees(courant, tension, temperature, SOH, SOC_simu);
}


/*int main() {
    setup();
    printf("Fin du programme\n");
    return 0;
}*/
