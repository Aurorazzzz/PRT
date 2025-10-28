#include "Read_Write.h"

void calcul_SOH(){
    // Initialisation des entrées
    const float *courant;
    const float *tension;
    const float *temperature;
    const float *SOH;
    const float *SOC_simu;

    // Initialisation des coefficients du filtre 
    const float a_filtre[] = {1, -0.969067417193793 };
    const float b_filtre[] = {0.0154662914031034,	0.0154662914031034 };

    Charge_donnees(&courant, &tension, &temperature, &SOH, &SOC_simu);

    Free_donnees(courant, tension, temperature, SOH, SOC_simu);
}