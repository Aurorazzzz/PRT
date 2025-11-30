#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Read_Write.h"
#include "sur_temperature.h"
#include "sur_tension.h"
#include "SOE.h"
#include "SOH.h"
#include "RUL.h"
#include "RINT.h"
#include "SOC.h"
#include "script_principal_step.h"

static double duree_en_seconde(clock_t t0, clock_t t1)
{
    return (double)(t1 - t0) / (double)CLOCKS_PER_SEC;
}

int main(void)
{
    // =====================================================================
    // 1) Chargement des données communes
    // =====================================================================
    const float *courant     = NULL;
    const float *tension     = NULL;
    const float *temperature = NULL;
    const float *SOH_vec     = NULL;
    const float *SOC_vec     = NULL;

    const int   NbIteration  = 1000000;   // 1 000 000 pas de 1 s
    const float periode_s    = 1.0f;      // cadence logique : 1 seconde

    Charge_donnees(&courant, &tension, &temperature, &SOH_vec, &SOC_vec);

    // Max par module (sur toutes les itérations)
    double temp_TEMP_max    = 0.0;
    double temp_TENSION_max = 0.0;
    double temp_SOE_max     = 0.0;
    double temp_SOH_max     = 0.0;
    double temp_RUL_max     = 0.0;
    double temp_RINT_max    = 0.0;
    double temp_SOC_max     = 0.0;

    // Dernière durée mesurée (pour la moyenne on ne garde que le cumul)
    double temp_TEMP_last   = 0.0;
    double temp_TENSION_last= 0.0;
    double temp_SOE_last    = 0.0;
    double temp_SOH_last    = 0.0;
    double temp_RUL_last    = 0.0;
    double temp_RINT_last   = 0.0;
    double temp_SOC_last    = 0.0;

    // =====================================================================
    // 2) Allocation des vecteurs de résultats
    // =====================================================================
    float *vect_T2_temp        = (float*)malloc(NbIteration * sizeof(float));
    float *vect_alerte_temp    = (float*)malloc(NbIteration * sizeof(float));

    float *vect_U_tension      = (float*)malloc(NbIteration * sizeof(float));
    float *vect_alerte_tension = (float*)malloc(NbIteration * sizeof(float));

    float *vect_SOE            = (float*)malloc(NbIteration * sizeof(float));
    float *vect_SOH            = (float*)malloc(NbIteration * sizeof(float));
    float *vect_RUL            = (float*)malloc(NbIteration * sizeof(float));
    float *vect_RINT           = (float*)malloc(NbIteration * sizeof(float));
    float *vect_SOC            = (float*)malloc(NbIteration * sizeof(float));

    // Pour information sur le temps d'exécution de chaque pas de 1 s
    float *vect_temps_cycle    = (float*)malloc(NbIteration * sizeof(float));

    if (!vect_T2_temp || !vect_alerte_temp ||
        !vect_U_tension || !vect_alerte_tension ||
        !vect_SOE || !vect_SOH || !vect_RUL || !vect_RINT ||
        !vect_temps_cycle || !vect_SOC)
    {
        perror("Erreur allocation vecteurs resultat");
        free(vect_T2_temp);
        free(vect_alerte_temp);
        free(vect_U_tension);
        free(vect_alerte_tension);
        free(vect_SOE);
        free(vect_SOH);
        free(vect_RUL);
        free(vect_RINT);
        free(vect_temps_cycle);
        free(vect_SOC);
        Free_donnees(courant, tension, temperature, SOH_vec, SOC_vec);
        return 1;
    }

    // =====================================================================
    // 3) Initialisation des contextes
    // =====================================================================
    TEMP_Context temp_ctx;
    TENSION_Context tens_ctx;
    SOE_Context soe_ctx;
    SOH_Context soh_ctx;
    RUL_Context rul_ctx;
    RINT_Context rint_ctx;
    SOC_Context soc_ctx;

    TEMP_init(&temp_ctx);
    TENSION_init(&tens_ctx);
    SOE_init(&soe_ctx);
    SOH_init(&soh_ctx);
    RUL_init(&rul_ctx);
    RINT_init(&rint_ctx);
    SOC_init(&soc_ctx);

    printf("========= Execution des modules (step) dans UNE boucle cadencee a 1 s =========\n");

    // =====================================================================
    // 4) Variables de mesure des temps CPU
    // =====================================================================
    double temps_TEMP        = 0.0;
    double temps_TENSION     = 0.0;
    double temps_SOE         = 0.0;
    double temps_SOH         = 0.0;
    double temps_RUL         = 0.0;
    double temps_RINT        = 0.0;
    double temps_SOC         = 0.0;

    double temps_cycle_total = 0.0;
    double temps_cycle_max   = 0.0;

    // =====================================================================
    // 5) Boucle principale unique, cadence "logique" de 1 seconde
    // =====================================================================
    for (int k = 0; k < NbIteration; ++k)
    {
        // Début du cycle de 1 s (en temps CPU)
        clock_t t_cycle0 = clock();

        float I_mes   = courant[k];
        float U_mes   = tension[k];
        float T_mes   = temperature[k];
        float SOC_k   = SOC_vec[k];
        float SOH_k   = SOH_vec[k];

        // -----------------------------------------------------------------
        // a) Module TEMPERATURE
        // -----------------------------------------------------------------
        {
            clock_t t0 = clock();

            int   alerte = 0;
            float T2 = TEMP_step(&temp_ctx, I_mes, T_mes, &alerte);

            clock_t t1 = clock();
            temp_TEMP_last = duree_en_seconde(t0, t1);
            temps_TEMP += temp_TEMP_last;
            if (temp_TEMP_last > temp_TEMP_max) temp_TEMP_max = temp_TEMP_last;

            vect_T2_temp[k]     = T2;
            vect_alerte_temp[k] = (float)alerte;
        }

        // -----------------------------------------------------------------
        // b) Module TENSION
        // -----------------------------------------------------------------
        {
            clock_t t0 = clock();

            int   alerte = 0;
            int   etat   = 1;       // décharge
            float I_sim  = -I_mes;

            float U_model = TENSION_step(&tens_ctx,
                                         I_sim,
                                         SOC_k,
                                         U_mes,
                                         etat,
                                         &alerte);

            clock_t t1 = clock();
            temp_TENSION_last = duree_en_seconde(t0, t1);
            temps_TENSION += temp_TENSION_last;
            if (temp_TENSION_last > temp_TENSION_max) temp_TENSION_max = temp_TENSION_last;

            vect_U_tension[k]      = U_model;
            vect_alerte_tension[k] = (float)alerte;
        }

        // -----------------------------------------------------------------
        // c) Module SOE
        // -----------------------------------------------------------------
        {
            clock_t t0 = clock();

            float soe_val = SOE_step(SOC_k, SOH_k, &soe_ctx);

            clock_t t1 = clock();
            temp_SOE_last = duree_en_seconde(t0, t1);
            temps_SOE += temp_SOE_last;
            if (temp_SOE_last > temp_SOE_max) temp_SOE_max = temp_SOE_last;

            vect_SOE[k] = soe_val;
        }

        // -----------------------------------------------------------------
        // d) Module SOH
        // -----------------------------------------------------------------
        {
            clock_t t0 = clock();

            float soh_val = SOH_step(&soh_ctx, I_mes, SOC_k);

            clock_t t1 = clock();
            temp_SOH_last = duree_en_seconde(t0, t1);
            temps_SOH += temp_SOH_last;
            if (temp_SOH_last > temp_SOH_max) temp_SOH_max = temp_SOH_last;

            vect_SOH[k] = soh_val;
        }

        // -----------------------------------------------------------------
        // e) Module RUL
        // -----------------------------------------------------------------
        {
            clock_t t0 = clock();

            float RUL_corrige = RUL_step(&rul_ctx, SOH_k, SOC_k);

            clock_t t1 = clock();
            temp_RUL_last = duree_en_seconde(t0, t1);
            temps_RUL += temp_RUL_last;
            if (temp_RUL_last > temp_RUL_max) temp_RUL_max = temp_RUL_last;

            vect_RUL[k] = RUL_corrige;
        }

        // -----------------------------------------------------------------
        // f) Module RINT
        // -----------------------------------------------------------------
        {
            clock_t t0 = clock();

            float SOHR_dummy;
            float I_eff = -I_mes;

            float Rint = RINT_step(&rint_ctx,
                                   U_mes,
                                   I_eff,
                                   SOC_k,
                                   &SOHR_dummy);

            clock_t t1 = clock();
            temp_RINT_last = duree_en_seconde(t0, t1);
            temps_RINT += temp_RINT_last;
            if (temp_RINT_last > temp_RINT_max) temp_RINT_max = temp_RINT_last;

            vect_RINT[k] = Rint;
        }

        // -----------------------------------------------------------------
        // g) Module SOC (estimation du SOC par LSTM + Kalman)
        // -----------------------------------------------------------------
        {
            clock_t t0 = clock();

            float SOC_est = SOC_step(&soc_ctx,
                                     I_mes,
                                     U_mes,
                                     T_mes,
                                     SOH_k);

            clock_t t1 = clock();
            temp_SOC_last = duree_en_seconde(t0, t1);
            temps_SOC += temp_SOC_last;
            if (temp_SOC_last > temp_SOC_max) temp_SOC_max = temp_SOC_last;

            vect_SOC[k] = SOC_est;
        }

        // -----------------------------------------------------------------
        // Fin du cycle de 1 s (en temps CPU)
        // -----------------------------------------------------------------
        clock_t t_cycle1 = clock();
        double duree_cycle = duree_en_seconde(t_cycle0, t_cycle1);
        temps_cycle_total += duree_cycle;
        if (duree_cycle > temps_cycle_max) temps_cycle_max = duree_cycle;

        // On mémorise le temps CPU utilisé pour ce pas de 1 s
        vect_temps_cycle[k] = (float)duree_cycle;
    }

    // =====================================================================
    // 6) Bilan des temps CPU
    // =====================================================================
    double temps_moyen_cycle   = temps_cycle_total / (double)NbIteration;
    double charge_cpu_pour_1Hz = (temps_moyen_cycle / (double)periode_s) * 100.0;

    printf("\n======================== BILAN DES TEMPS CPU ========================\n");
    printf("%-12s | %-12s | %-12s | %-12s\n",
           "Module", "Cumul (s)", "Moyen (us)", "Max (us)");
    printf("---------------------------------------------------------------------\n");

    printf("%-12s | %12.6f | %12.2f | %12.2f\n",
           "TEMP",
           temps_TEMP,
           (temps_TEMP / NbIteration) * 1e6,
           temp_TEMP_max * 1e6);

    printf("%-12s | %12.6f | %12.2f | %12.2f\n",
           "TENSION",
           temps_TENSION,
           (temps_TENSION / NbIteration) * 1e6,
           temp_TENSION_max * 1e6);

    printf("%-12s | %12.6f | %12.2f | %12.2f\n",
           "SOE",
           temps_SOE,
           (temps_SOE / NbIteration) * 1e6,
           temp_SOE_max * 1e6);

    printf("%-12s | %12.6f | %12.2f | %12.2f\n",
           "SOH",
           temps_SOH,
           (temps_SOH / NbIteration) * 1e6,
           temp_SOH_max * 1e6);

    printf("%-12s | %12.6f | %12.2f | %12.2f\n",
           "RUL",
           temps_RUL,
           (temps_RUL / NbIteration) * 1e6,
           temp_RUL_max * 1e6);

    printf("%-12s | %12.6f | %12.2f | %12.2f\n",
           "RINT",
           temps_RINT,
           (temps_RINT / NbIteration) * 1e6,
           temp_RINT_max * 1e6);

    printf("%-12s | %12.6f | %12.2f | %12.2f\n",
           "SOC",
           temps_SOC,
           (temps_SOC / NbIteration) * 1e6,
           temp_SOC_max * 1e6);

    printf("---------------------------------------------------------------------\n");
    printf("Cycle 1 s : cumul = %10.6f s | moyen = %10.9f s | max = %10.9f s\n",
           temps_cycle_total, temps_moyen_cycle, temps_cycle_max);
    printf("Charge CPU pour cadence 1 Hz : %.3f %%\n", charge_cpu_pour_1Hz);
    printf("=====================================================================\n");

    // =====================================================================
    // 7) Écriture des résultats
    // =====================================================================
    Ecriture_result(vect_T2_temp,        NbIteration, "TEMPERATURE_vscode");
    Ecriture_result(vect_alerte_temp,    NbIteration, "ALERTE_TEMPERATURE_vscode");
    Ecriture_result(vect_U_tension,      NbIteration, "TENSION_vscode");
    Ecriture_result(vect_alerte_tension, NbIteration, "ALERTE_TENSION_vscode");
    Ecriture_result(vect_SOE,            NbIteration, "SOE_vscode");
    Ecriture_result(vect_SOH,            NbIteration, "SOH_vscode");
    Ecriture_result(vect_RUL,            NbIteration, "RUL_vscode");
    Ecriture_result(vect_RINT,           NbIteration, "RINT_vscode");
    Ecriture_result(vect_SOC,            NbIteration, "SOC_vscode");
    Ecriture_result(vect_temps_cycle,    NbIteration, "TEMPS_CYCLE_CPU");

    // =====================================================================
    // 8) Nettoyage des données d'entrée
    // =====================================================================
    Free_donnees(courant, tension, temperature, SOH_vec, SOC_vec);

    free(vect_T2_temp);
    free(vect_alerte_temp);
    free(vect_U_tension);
    free(vect_alerte_tension);
    free(vect_SOE);
    free(vect_SOH);
    free(vect_RUL);
    free(vect_RINT);
    free(vect_SOC);
    free(vect_temps_cycle);

    return 0;
}
