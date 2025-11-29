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
#include "script_principal_step.h"

// ---------------------------------------------------------------------------
// Petit helper pour mesurer le temps d'un bloc de code
// ---------------------------------------------------------------------------
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

    // Pour information sur le temps d'exécution de chaque pas de 1 s
    float *vect_temps_cycle    = (float*)malloc(NbIteration * sizeof(float));

    if (!vect_T2_temp || !vect_alerte_temp ||
        !vect_U_tension || !vect_alerte_tension ||
        !vect_SOE || !vect_SOH || !vect_RUL || !vect_RINT ||
        !vect_temps_cycle)
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

    TEMP_init(&temp_ctx);
    TENSION_init(&tens_ctx);
    SOE_init(&soe_ctx);
    SOH_init(&soh_ctx);
    RUL_init(&rul_ctx);
    RINT_init(&rint_ctx);

    printf("========= Execution des modules (step) dans UNE boucle cadencee a 1 s =========\n");

    // =====================================================================
    // 4) Variables de mesure des temps CPU
    // =====================================================================
    double temps_TEMP     = 0.0;
    double temps_TENSION  = 0.0;
    double temps_SOE      = 0.0;
    double temps_SOH      = 0.0;
    double temps_RUL      = 0.0;
    double temps_RINT     = 0.0;
    double temps_cycle_total = 0.0;

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
            float T2     = TEMP_step(&temp_ctx, I_mes, T_mes, &alerte);

            clock_t t1 = clock();
            temps_TEMP += duree_en_seconde(t0, t1);

            vect_T2_temp[k]     = T2;
            vect_alerte_temp[k] = (float)alerte;
        }

        // -----------------------------------------------------------------
        // b) Module TENSION
        //     Ici on garde etat = 1 (décharge) comme dans votre version
        //     précédente. Vous pourrez plus tard brancher la vraie détection
        //     de phase charge/decharge si souhaité.
        // -----------------------------------------------------------------
        {
            clock_t t0 = clock();

            int   alerte = 0;
            int   etat   = 1;          // 1 = décharge, 0 = charge
            float I_sim  = -I_mes;     // même convention que votre ancien setup

            float U_model = TENSION_step(&tens_ctx,
                                         I_sim,
                                         SOC_k,
                                         U_mes,
                                         etat,
                                         &alerte);

            clock_t t1 = clock();
            temps_TENSION += duree_en_seconde(t0, t1);

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
            temps_SOE += duree_en_seconde(t0, t1);

            vect_SOE[k] = soe_val;
        }

        // -----------------------------------------------------------------
        // d) Module SOH
        // -----------------------------------------------------------------
        {
            clock_t t0 = clock();

            float soh_val = SOH_step(&soh_ctx, I_mes, SOC_k);

            clock_t t1 = clock();
            temps_SOH += duree_en_seconde(t0, t1);

            vect_SOH[k] = soh_val;
        }

        // -----------------------------------------------------------------
        // e) Module RUL
        // -----------------------------------------------------------------
        {
            clock_t t0 = clock();

            float RUL_corrige = RUL_step(&rul_ctx, SOH_k, SOC_k);

            clock_t t1 = clock();
            temps_RUL += duree_en_seconde(t0, t1);

            vect_RUL[k] = RUL_corrige;
        }

        // -----------------------------------------------------------------
        // f) Module RINT
        // -----------------------------------------------------------------
        {
            clock_t t0 = clock();

            float SOHR_dummy;
            float I_effectif = -I_mes;   // même convention que dans RINT_Theo

            float Rint = RINT_step(&rint_ctx,
                                   U_mes,
                                   I_effectif,
                                   SOC_k,
                                   &SOHR_dummy);

            clock_t t1 = clock();
            temps_RINT += duree_en_seconde(t0, t1);

            vect_RINT[k] = Rint;
        }

        // -----------------------------------------------------------------
        // Fin du cycle de 1 s (en temps CPU)
        // -----------------------------------------------------------------
        clock_t t_cycle1 = clock();
        double duree_cycle = duree_en_seconde(t_cycle0, t_cycle1);
        temps_cycle_total += duree_cycle;

        // On mémorise le temps CPU utilisé pour ce pas de 1 s
        vect_temps_cycle[k] = (float)duree_cycle;

        // Si vous voulez voir quelques valeurs :
        /*
        if (k < 10) {
            printf("k=%d, temps cycle = %.6f s\n", k, duree_cycle);
        }
        */
    }

    // =====================================================================
    // 6) Bilan des temps CPU
    // =====================================================================
    double temps_total_modules = temps_TEMP + temps_TENSION + temps_SOE
                               + temps_SOH + temps_RUL + temps_RINT;
    double temps_moyen_cycle   = temps_cycle_total / (double)NbIteration;
    double charge_cpu_pour_1Hz = (temps_moyen_cycle / periode_s) * 100.0;

    printf("-----------------------------------------------------------------\n");
    printf("Temps cumules par module (sur %d pas de 1 s):\n", NbIteration);
    printf("  TEMP     : %.6f s\n", temps_TEMP);
    printf("  TENSION  : %.6f s\n", temps_TENSION);
    printf("  SOE      : %.6f s\n", temps_SOE);
    printf("  SOH      : %.6f s\n", temps_SOH);
    printf("  RUL      : %.6f s\n", temps_RUL);
    printf("  RINT     : %.6f s\n", temps_RINT);
    printf("  Somme modules      : %.6f s\n", temps_total_modules);
    printf("  Temps total cycles : %.6f s\n", temps_cycle_total);
    printf("  Temps CPU moyen par pas de 1 s : %.9f s\n", temps_moyen_cycle);
    printf("  Charge CPU pour cadence 1 Hz   : %.3f %%\n", charge_cpu_pour_1Hz);
    printf("-----------------------------------------------------------------\n");

    // =====================================================================
    // 7) Écriture des résultats
    // =====================================================================
    Ecriture_result(vect_T2_temp,        NbIteration, "TEMPERATURE_MODEL_PC_result");
    Ecriture_result(vect_alerte_temp,    NbIteration, "ALERTE_TEMPERATURE_PC_result");
    Ecriture_result(vect_U_tension,      NbIteration, "TENSION_MODEL_raspi_result2");
    Ecriture_result(vect_alerte_tension, NbIteration, "ALERTE_TENSION_raspi_result");
    Ecriture_result(vect_SOE,            NbIteration, "SOE_raspi_result");
    Ecriture_result(vect_SOH,            NbIteration, "SOH_ordi");
    Ecriture_result(vect_RUL,            NbIteration, "RUL_raspi_result");
    Ecriture_result(vect_RINT,           NbIteration, "RINT_ordi");
    Ecriture_result(vect_temps_cycle,    NbIteration, "TEMPS_CYCLE_CPU");

    // =====================================================================
    // 8) Nettoyage des données d'entrée
    // =====================================================================
    Free_donnees(courant, tension, temperature, SOH_vec, SOC_vec);

    return 0;
}
