#ifndef READ_WRITE_H
#define READ_WRITE_H

#include <stdio.h>
#include <stdlib.h>

void Charge_donnees (const float **courant,const float **tension, const float **temperature, const float **SOH, const float **SOC);
void Free_donnees (const float *courant, const float *tension, const float *temperature, const float *SOH, const float *SOC);
int Ecriture_result(float *data, const int NbIteration, const char *nom_fichier);
#endif 