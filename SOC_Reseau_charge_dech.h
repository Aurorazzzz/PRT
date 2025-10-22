#ifndef SOC_AURORE_H
#define SOC_AURORE_H

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/* Prototypes generated from SOC_Aurore.c */

void setup(void);

void estimationSOC(float courant, float tension, float temperature, float SOH, const float moins_eta_sur_Q, float *pointeur_Pk, float *pointeur_SOC,
                   const float dt, const float Qk, const float Rk, int tailleEntree, int tailleReseau, int tailleSortie, float *pointeur_sortie_LSTM,
                   float *pointeur_xt, float *pointeur_ct, float *pointeur_ht,
                   float *pointeur_it, float *pointeur_ft, float *pointeur_gt, float *pointeur_ot,
                   const float *pointeur_Wi,const float *pointeur_Wf,const float *pointeur_Wg,const float *pointeur_Wo,
                   const float *pointeur_Ri,const float *pointeur_Rf,const float *pointeur_Rg,const float *pointeur_Ro,
                   const float *pointeur_bi,const float *pointeur_bf,const float *pointeur_bg,const float *pointeur_bo,
                   const float *pointeur_MOY,const float *pointeur_ECART_TYPE,const float *pointeur_WFC,const float *pointeur_bFC,
                   float *pointeur_vect_intermediaire_1, float *pointeur_vect_intermediaire_2);

void predictionLSTM(int tailleEntree, int tailleReseau, int tailleSortie, float *pointeur_sortie_LSTM, float *pointeur_xt, float *pointeur_ct, float *pointeur_ht,
                    float *pointeur_it, float *pointeur_ft, float *pointeur_gt, float *pointeur_ot,
                    const float *pointeur_Wi,const float *pointeur_Wf,const float *pointeur_Wg,const float *pointeur_Wo,
                    const float *pointeur_Ri,const float *pointeur_Rf,const float *pointeur_Rg,const float *pointeur_Ro,
                    const float *pointeur_bi,const float *pointeur_bf,const float *pointeur_bg,const float *pointeur_bo,
                    const float *pointeur_MOY,const float *pointeur_ECART_TYPE,const float *pointeur_WFC,const float *pointeur_bFC,
                    float *pointeur_vect_intermediaire_1, float *pointeur_vect_intermediaire_2);

void sigma_g_sigmoide(int x_size, float *pointeur_entree, float *pointeur_sortie_sig);
void sigma_c_tanh(int x_size, float *pointeur_entree, float *pointeur_sortie_tanh);

void MatriceFoisVecteur(int nbLignesMatrice, int tailleVecteur, const float *pointeur_matrice, float *pointeur_vecteur, float *pointeur_sortie_prodMat);

void Charge_donnees (float **courant, float **tension, float **temperature, float **SOH, float **SOC);

void Free_donnees (float *courant, float *tension, float *temperature, float *SOH, float *SOC);

int Ecriture_SOC(float *SOC, const int NbIteration);

#endif /* SOC_AURORE_H */
