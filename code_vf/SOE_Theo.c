#include <stdio.h>
#include <stdlib.h>
#include "SOE_Theo.h"
#include "Read_Write.h"

// ============================================================================
// Interpolation linéaire rapide 1D — version C identique à interp1rapide.m MATLAB
// Entrées :
//   x[]  : tableau des abscisses (longueur n, croissant)
//   y[]  : tableau des ordonnées associées
//   n    : taille du tableau
//   x_req: valeur où interpoler
// Sortie : valeur interpolée
// ============================================================================

/*float interp1rapide(const float *x, const float *y, int n, float x_req)
{
    int indice = -1; // empty

    // liste_indice = find(x_req > x) ; indice = max(liste_indice)
    for (int i = 0; i < n; ++i) {
        if (x_req > x[i]) indice = i;  // on garde le DERNIER i qui vérifie
    }

    float coord;
    if (indice >= 0 && indice < n - 1) {
        coord = (x_req - x[indice]) / (x[indice + 1] - x[indice]);
    } else if (indice < 0) {
        indice = 1;
        coord = 0.0f;
    } else if (indice >= n) {
        indice = n - 1;
        coord = 1.0f;
    } else {
        indice = n;
        coord = 0.0f;
    }

    // sortie = (1-coord)*y(indice) + coord*y(indice+1)
    return (1.0f - coord) * y[indice] + coord * y[indice + 1];
}*/

// ============================================================================
// Fonction principale : estimation du SOE
// Entrées :
//   SOC, SOH                : état de charge et de santé (0–1)
//   moins_eta_sur_Q         : 1/(rendement*capacité) [1/Coulomb]
//   X_OCV[], LOI_INTEG_OCV_DECHARGE[] : tables du modèle
//   n                       : taille de ces tables
// Sortie : SOE_pred (Joules ou unité cohérente avec la table)
// ============================================================================
float estimation_SOE(float SOC, float SOH, float moins_eta_sur_Q,
                            const float *X_OCV, const float *LOI_INTEG_OCV_DECHARGE, int n)
{
    // garde-fous
    if (SOC < 0.0f) SOC = 0.0f;
    if (SOC > 1.0f) SOC = 1.0f;
    if (SOH < 0.0f) SOH = 0.0f;
    if (SOH > 1.0f) SOH = 1.0f;
    if (moins_eta_sur_Q == 0.0f) return 0.0f;

    // intégrale de courant prédite (Coulombs)
    float integrale_courant_pred = (1.0f / moins_eta_sur_Q) * SOH * SOC;

    // “tension moyenne intégrée” interpolée depuis la table
    float ocv_moyenne = interp1Drapide(X_OCV, LOI_INTEG_OCV_DECHARGE, n, SOC);
    //float ocv_moyenne = 1.0f; // Pour tester sans la table

    // SOE (énergie) — unités cohérentes avec la table
    return ocv_moyenne * integrale_courant_pred;
}

void SOE_setup(void)
{
    // === Entrées (buffers fournis par Charge_donnees) ===
    const float *courant    = NULL;
    const float *tension    = NULL;
    const float *temperature= NULL;
    const float *SOH        = NULL;
    const float *SOC_simu   = NULL;

    //Initialisation du nombre de données simulées
    const int NbIteration = 1000000;

    //Initialisation du vecteur contenant le SOC :
    float *vecteur_SOE = malloc(NbIteration * sizeof(float));
    if (!vecteur_SOE) {
        perror("Erreur allocation mémoire");
        return;
    }

    // === Comptage coulombmétrique ===
    float moins_eta_sur_Q = 2.300303904920101e-04f;
    float integrale_courant_neuf = 1.0f / moins_eta_sur_Q; // utile si besoin ailleurs

    // === Tables OCV intégrées (104 points) ===
    static const float X_OCV[104] = {
        0, 0.0130000000000000, 0.0200000000000000, 0.0300000000000000, 0.0400000000000000, 0.0500000000000000, 0.0600000000000000, 0.0700000000000000, 0.0800000000000000, 0.0900000000000000, 0.100000000000000, 0.110000000000000, 0.120000000000000, 0.130000000000000, 0.140000000000000, 0.150000000000000, 0.160000000000000, 0.170000000000000, 0.180000000000000, 0.190000000000000, 0.200000000000000, 0.210000000000000, 0.220000000000000, 0.230000000000000, 0.240000000000000, 0.250000000000000, 0.260000000000000, 0.270000000000000, 0.280000000000000, 0.290000000000000, 0.300000000000000, 0.310000000000000, 0.320000000000000, 0.330000000000000, 0.340000000000000, 0.350000000000000, 0.360000000000000, 0.370000000000000, 0.380000000000000, 0.390000000000000, 0.400000000000000, 0.410000000000000, 0.420000000000000, 0.430000000000000, 0.440000000000000, 0.450000000000000, 0.460000000000000, 0.470000000000000, 0.480000000000000, 0.490000000000000, 0.500000000000000, 0.510000000000000, 0.520000000000000, 0.530000000000000, 0.540000000000000, 0.550000000000000, 0.560000000000000, 0.570000000000000, 0.580000000000000, 0.590000000000000, 0.600000000000000, 0.610000000000000, 0.620000000000000, 0.630000000000000, 0.640000000000000, 0.650000000000000, 0.660000000000000, 0.670000000000000, 0.680000000000000, 0.690000000000000, 0.700000000000000, 0.710000000000000, 0.720000000000000, 0.730000000000000, 0.740000000000000, 0.750000000000000, 0.760000000000000, 0.770000000000000, 0.780000000000000, 0.790000000000000, 0.800000000000000, 0.810000000000000, 0.820000000000000, 0.830000000000000, 0.840000000000000, 0.850000000000000, 0.860000000000000, 0.870000000000000, 0.880000000000000, 0.890000000000000, 0.900000000000000, 0.910000000000000, 0.920000000000000, 0.930000000000000, 0.940000000000000, 0.950000000000000, 0.960000000000000, 0.970000000000000, 0.975000000000000, 0.980000000000000, 0.981000000000000, 0.990000000000000, 0.998000000000000, 1
    };
    static const float LOI_INTEG_OCV_DECHARGE[104] = {
        2.17810726521397, 2.42387651090271, 2.52340973680752, 2.61810811995897, 2.68830211253072, 2.74359059498592, 2.78879588809875, 2.82666386791756, 2.85892699638151, 2.88674873587964, 2.91099216582562, 2.93230092085619, 2.95118693957418, 2.96805445200887, 2.98323334088822, 2.99698282496203, 3.00951383340767, 3.02099577240751, 3.03157050451826, 3.04135200243041, 3.05043125074092, 3.05889154736900, 3.06679943747442, 3.07421029433268, 3.08117465468151, 3.08773344544836, 3.09392632129709, 3.09978513541024, 3.10533809272911, 3.11061286868298, 3.11563458838623, 3.12042148478899, 3.12499478136048, 3.12937079285950, 3.13356656210501, 3.13759596424894, 3.14147273931224, 3.14520869191849, 3.14881474321317, 3.15230058057039, 3.15567400082808, 3.15894253028603, 3.16211476725150, 3.16519395741560, 3.16818841327157, 3.17110283618178, 3.17393913430038, 3.17670384504597, 3.17940069811906, 3.18203251803023, 3.18460238097567, 3.18711415999426, 3.18956997325686, 3.19197239919673, 3.19432416892391, 3.19662621118576, 3.19888148091879, 3.20109253142139, 3.20326007104181, 3.20538549032296, 3.20747073935636, 3.20951817680704, 3.21152878261973, 3.21350399988768, 3.21544549827404, 3.21735409303498, 3.21923181425047, 3.22107908554545, 3.22289697440416, 3.22468724981292, 3.22644939766687, 3.22818518684065, 3.22989427830465, 3.23157744502623, 3.23323652892598, 3.23487132683029, 3.23648286281331, 3.23807107816518, 3.23963706601348, 3.24118185321509, 3.24270541010895, 3.24420955500570, 3.24569407112564, 3.24715939970186, 3.24860685797354, 3.25003637676908, 3.25144889436468, 3.25284569654290, 3.25422795676839, 3.25559540209492, 3.25694900283186, 3.25829111445285, 3.25962184504910, 3.26094286505299, 3.26225615840580, 3.26356353177906, 3.26487058920003, 3.26619163131266, 3.26688951827563, 3.26758740523860, 3.26776083629660, 3.26932171581863, 3.27335170386112, 3.27435920087174
    };

    Charge_donnees(&courant, &tension, &temperature, &SOH, &SOC_simu);

        // === Calcul du SOE pour chaque échantillon ===
    for (size_t i = 0; i < NbIteration; ++i) {
        float SOC_i = SOC_simu[i];  // SOC simulé à l’instant i
        float SOH_i = SOH[i];       // SOH courant (si constant, le buffer contiendra la même valeur)

        float SOE_i = estimation_SOE(SOC_i, SOH_i, moins_eta_sur_Q,
                                     X_OCV, LOI_INTEG_OCV_DECHARGE, 104);

        vecteur_SOE[i] = SOE_i;
        // Affichage (adapte selon ton besoin : sauvegarde, log, etc.)
        //printf("i=%zu  SOC=%.6f  SOH=%.6f  SOE=%.6f\n", i, SOC_i, SOH_i, SOE_i);
    }

    Ecriture_result(vecteur_SOE, NbIteration, "SOE_raspi_result");

    Free_donnees(courant, tension, temperature, SOH, SOC_simu);


}

/*int main() {
    setup();
    printf("Fin du programme !\n");
    return 0;
}*/
    //protect 