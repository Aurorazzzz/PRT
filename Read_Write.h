#ifndef READ_WRITE_H
#define READ_WRITE_H

#include <stdio.h>
#include <stdlib.h>

void Charge_donnees (const float **courant,const float **tension, const float **temperature, const float **SOH, const float **SOC);
void Free_donnees (const float *courant, const float *tension, const float *temperature, const float *SOH, const float *SOC);
int Ecriture_result(float *data, const int NbIteration, const char *nom_fichier);
float interp1Drapide(const float *x_tab, const float *y_tab, int n, float x);
void detection_charge_decharge(float courant,float tampon_charge_decharge[60],int *etat,int *etat_precedent);
#endif 