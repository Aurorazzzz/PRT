#include <stdio.h>
#include <stdlib.h>
#include "SOE_Theo.h"
#include "Read_Write.h"


// ============================================================================
// Interpolation linéaire rapide 1D : équivalent de "interp1rapide" MATLAB
// Entrées :
//   x_tab[] : tableau des abscisses (X_OCV)
//   y_tab[] : tableau des ordonnées (LOI_INTEG_OCV_DECHARGE)
//   n       : taille du tableau
//   x       : valeur d’interpolation (ici le SOC)
// Sortie : valeur interpolée
// ============================================================================
float interp1rapide(const float *x_tab, const float *y_tab, int n, float x)
{
    if (n <= 0) return 0.0f;
    if (x <= x_tab[0])     return y_tab[0];
    if (x >= x_tab[n - 1]) return y_tab[n - 1];

    for (int i = 0; i < n - 1; i++) {
        if (x >= x_tab[i] && x <= x_tab[i + 1]) {
            float dx = x_tab[i + 1] - x_tab[i];
            float dy = y_tab[i + 1] - y_tab[i];
            // éviter division par 0 si deux x identiques
            if (dx == 0.0f) return y_tab[i];
            float t = (x - x_tab[i]) / dx;
            return y_tab[i] + t * dy;
        }
    }
    return y_tab[n - 1];  // sécurité
}

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
    float ocv_moyenne = interp1rapide(X_OCV, LOI_INTEG_OCV_DECHARGE, n, SOC);

    // SOE (énergie) — unités cohérentes avec la table
    return ocv_moyenne * integrale_courant_pred;
}

void setup(void)
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
    // float integrale_courant_neuf = 1.0f / moins_eta_sur_Q; // utile si besoin ailleurs

    // === Tables OCV intégrées (104 points) ===
    static const float X_OCV[104] = {
        0.000000, 0.013000, 0.020000, 0.030000, 0.040000, 0.050000, 0.060000, 0.070000, 0.080000, 0.090000,
        0.100000, 0.110000, 0.120000, 0.130000, 0.140000, 0.150000, 0.160000, 0.170000, 0.180000, 0.190000,
        0.200000, 0.210000, 0.220000, 0.230000, 0.240000, 0.250000, 0.260000, 0.270000, 0.280000, 0.290000,
        0.300000, 0.310000, 0.320000, 0.330000, 0.340000, 0.350000, 0.360000, 0.370000, 0.380000, 0.390000,
        0.400000, 0.410000, 0.420000, 0.430000, 0.440000, 0.450000, 0.460000, 0.470000, 0.480000, 0.490000,
        0.500000, 0.510000, 0.520000, 0.530000, 0.540000, 0.550000, 0.560000, 0.570000, 0.580000, 0.590000,
        0.600000, 0.610000, 0.620000, 0.630000, 0.640000, 0.650000, 0.660000, 0.670000, 0.680000, 0.690000,
        0.700000, 0.710000, 0.720000, 0.730000, 0.740000, 0.750000, 0.760000, 0.770000, 0.780000, 0.790000,
        0.800000, 0.810000, 0.820000, 0.830000, 0.840000, 0.850000, 0.860000, 0.870000, 0.880000, 0.890000,
        0.900000, 0.910000, 0.920000, 0.930000, 0.940000, 0.950000, 0.960000, 0.970000, 0.980000, 0.985000,
        0.990000, 0.992000, 0.995000, 0.997000, 0.998000, 0.999000, 0.999500, 1.000000
    };
    static const float LOI_INTEG_OCV_DECHARGE[104] = {
        2.178107, 2.423877, 2.523410, 2.618108, 2.688302, 2.743591, 2.788796, 2.826692, 2.859379, 2.888574,
        2.915472, 2.940917, 2.965908, 2.990597, 3.014939, 3.038733, 3.061955, 3.084818, 3.107413, 3.129903,
        3.152403, 3.174962, 3.197690, 3.220682, 3.243977, 3.267661, 3.291770, 3.316344, 3.341397, 3.366959,
        3.393056, 3.419712, 3.446951, 3.474795, 3.503264, 3.532377, 3.562152, 3.592604, 3.623747, 3.655595,
        3.688159, 3.721452, 3.755485, 3.790269, 3.825814, 3.862132, 3.899231, 3.937122, 3.975813, 4.015314,
        4.055634, 4.096781, 4.138764, 4.181591, 4.225271, 4.269813, 4.315225, 4.361515, 4.408692, 4.456764,
        4.505738, 4.555624, 4.606427, 4.658157, 4.710823, 4.764432, 4.818992, 4.874512, 4.930999, 4.988461,
        5.046905, 5.106339, 5.166771, 5.228206, 5.290653, 5.354119, 5.418612, 5.484139, 5.550707, 5.618324,
        5.686996, 5.756730, 5.827534, 5.899414, 5.972378, 6.046432, 6.121584, 6.197839, 6.275205, 6.353688,
        6.433295, 6.514034, 6.595910, 6.678931, 6.763105, 6.848438, 6.934938, 7.022612, 7.111467, 7.156240,
        7.201307, 7.224016, 7.247068, 7.259009, 7.265017, 7.269044, 7.271222, 7.272667
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

int main() {
    setup();
    printf("Fin du programme !\n");
    return 0;
}//protect 