#ifndef SOH_V1_H
#define SOH_V1_H

#include <stdbool.h>
#include <math.h>
#include <string.h>

void calcul_vecteur_SOH();

void calcul_SOH(float *integrale_courant, const float courant, bool changement_etat, const float dt, const float integrale_courant_neuf, 
    float *SOC_precedent, const float SOC, float *SOH, float *y_n_1, float *x_n_1, const double a_filtre[], const double b_filtre[]);

    void detection_charge_decharge(float *tampon_charge_decharge, const int taille_tampon, const float courant, bool *etat_precedent, bool *changement_etat);

float moyenne(float *tableau, const int taille);

#endif 