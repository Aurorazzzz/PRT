#include <stdio.h>
#include <time.h>
#include "script_principal.h"


// ---------------------------------------------------------------------------
// mesure le temps d'exécution d'une fonction void f(void)
// ---------------------------------------------------------------------------
static double mesurer_temps(void (*f)(void), const char *nom)
{
    clock_t t0 = clock();
    f();
    clock_t t1 = clock();

    double duree_s = (double)(t1 - t0) / (double)CLOCKS_PER_SEC;
    double duree_us = duree_s * 1e6;

    printf("Module %-20s : %10.6f s  (%10.0f µs)\n", nom, duree_s, duree_us);

    return duree_s;
}

static void dummy(void)
{
    // juste une petite boucle pour consommer un peu de temps
    volatile double s = 0.0;
    for (int i = 0; i < 10000000; ++i) {
        s += i * 0.000001;
    }
}


// ---------------------------------------------------------------------------
// main : lance 1 à 1 les codes et mesure les temps
// ---------------------------------------------------------------------------
int main(void)
{
    clock_t t_start = clock();

    double temps_total = 0.0;
    printf("===== Mesure des temps d'execution des modules =====\n");

    temps_total += mesurer_temps(dummy, "surveillance");
    temps_total += mesurer_temps(TEMP_setup, "surveillance_temperature");
    temps_total += mesurer_temps(TENSION_setup, "surveillance_tension_Theo");

    clock_t t_end = clock();
    double temps_global = (double)(t_end - t_start) / CLOCKS_PER_SEC;

    printf("--------------------------test--------------------------\n");
    printf("Temps total (modules) : %.6f s\n", temps_total);
    printf("Temps global programme : %.6f s\n", temps_global);

    return 0;
}
