#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "SOP.h"
#include "Read_Write.h"
#include <stdbool.h>

/* ========================================================================== */
/*  Interpolation rapide + dérivée (interp1rapide_der.m)                      */
/* ========================================================================== */
/* MATLAB : interp1rapide_der(x,y,x_req) retourne [sortie, der]              */
/*  - On suppose x croissant et de taille n                                   */
/* ========================================================================== */
static void interp1rapide_der(const float *x,
                              const float *y,
                              int          n,
                              float        x_req,
                              float       *sortie,
                              float       *der){
    int indice = -1;
    //printf("x_req=%f\n", x_req);
    /* liste_indice = find(x_req > x); indice = max(liste_indice) */
    for (int i = 0; i < n; ++i)
    {
        if (x_req > x[i]) indice = i;
    }
    //printf("indice=%d\n", indice);
    float coord;
    if (indice >= 0 && indice < n - 1)
    {
        /* cas "normal" */
        float dx = x[indice + 1] - x[indice];

        if (dx == 0.0f) dx = 1e-9f;
        coord  = (x_req - x[indice]) / dx;
        *der   = (y[indice + 1] - y[indice]) / dx;
    }
    else if (indice < 0)
    {
        /* x_req avant la première abscisse => indice = 0 en C */
        indice = 0;
        float dx = x[1] - x[0];
        if (dx == 0.0f) dx = 1e-9f;
        coord  = 0.0f;
        *der   = (y[1] - y[0]) / dx;
        //printf("dx=%f\n", dx);
        //printf("y=%f\n", y[1] - y[0]);
        //printf("deriv=%f\n", *der);
    }
    else
    {
        /* x_req au-delà de la dernière abscisse => indice = n-2 en C */
        indice = n - 2;
        float dx = x[indice + 1] - x[indice];
        if (dx == 0.0f) dx = 1e-9f;
        coord  = 1.0f;
        *der   = (y[indice + 1] - y[indice]) / dx;
    } 
    //printf("y=%f\n", y[indice]);
    *sortie = (1.0f - coord) * y[indice] + coord * y[indice + 1];
    //printf("sortie=%f\n", *sortie);
}

void simuler_horizon_batterie(float moins_eta_sur_Q,
                              float dt,
                              int   horizon,
                              float SOC_init,
                              float SOH,
                              const float parametre_therm[4],
                              float T1_init,
                              float T2_init,
                              float TAMB,
                              float Ir_init,
                              int   etat,
                              const float *X_OCV_dep,
                              const float *Y_OCV_dep_charge,
                              const float *Y_OCV_dep_decharge,
                              int   n_OCV,
                              float R1,
                              float C1,
                              float R0,
                              float I_candidat,
                              float SOC_minmax[2], // [min, max]
                              float T1_minmax[2],
                              float T2_minmax[2],
                              float U_minmax[2],
                              float *Ir_final)
{
    // États instantanés
    float SOC = SOC_init;
    float T1  = T1_init;
    float T2  = T2_init;
    float Ir  = Ir_init;

    // Init min/max avec l'état initial
    SOC_minmax[0] = SOC; SOC_minmax[1] = SOC;
    T1_minmax[0]  = T1;  T1_minmax[1]  = T1;
    T2_minmax[0]  = T2;  T2_minmax[1]  = T2;

    // Il faut aussi une tension initiale : on peut la calculer une fois
    float U0 = modele_tension_1RC_step(I_candidat, SOC, &Ir,
                                       etat,
                                       X_OCV_dep,
                                       Y_OCV_dep_charge,
                                       Y_OCV_dep_decharge,
                                       n_OCV,
                                       dt, R1, C1, R0);
    //printf("U0=%f\n", U0);
    U_minmax[0] = U0;
    U_minmax[1] = U0;

    // Boucle sur l'horizon (on a déjà traité le pas 0 avec U0, à vous de choisir si vous incluez ou pas k=0 dans horizon)
    for (int k = 1; k < horizon; ++k)
    {
        // 1) Avancer le SOC
        SOC = modele_SOC_CC_step(moins_eta_sur_Q, dt, SOC, I_candidat, SOH);
        //printf("SOC_ten=%f\n", SOC); 
        // 2) Avancer le thermique
        modele_thermique_foster_ordre_2_step(parametre_therm, I_candidat, dt, TAMB,
                                             &T1, &T2);

        // 3) Avancer la tension (Ir mis à jour dedans)
        float U = modele_tension_1RC_step(I_candidat, SOC, &Ir,
                                        etat,
                                          X_OCV_dep,
                                          Y_OCV_dep_charge,
                                          Y_OCV_dep_decharge,
                                          n_OCV,
                                         dt, R1, C1, R0);
        //printf("k=%f\n", k); 
        //printf("U_blo=%f\n", U);
        // 4) Mettre à jour les min/max

        if (SOC < SOC_minmax[0]) SOC_minmax[0] = SOC;
        if (SOC > SOC_minmax[1]) SOC_minmax[1] = SOC;

        if (T1 < T1_minmax[0]) T1_minmax[0] = T1;
        if (T1 > T1_minmax[1]) T1_minmax[1] = T1;

        if (T2 < T2_minmax[0]) T2_minmax[0] = T2;
        if (T2 > T2_minmax[1]) T2_minmax[1] = T2;

        if (U < U_minmax[0]) U_minmax[0] = U;
        if (U > U_minmax[1]) U_minmax[1] = U;
    }

    if (Ir_final) *Ir_final = Ir;
}

static float modele_SOC_CC_step(float moins_eta_sur_Q,
                                float dt,
                                float SOC_prev,
                                float I,
                                float SOH)
{
    // printf("SOC_prev=%f\n", SOC_prev);
    // printf("I=%f\n", I);
    // printf("dt=%f\n", dt);
    return SOC_prev - moins_eta_sur_Q * dt * I / SOH;
}

static void modele_thermique_foster_ordre_2_step(const float parametre[4],
                                                 float       I,
                                                 float       dt,
                                                 float       TAMB,
                                                 float      *T1,   // in/out
                                                 float      *T2)   // in/out
{
    float R1 = parametre[0];
    float C1 = parametre[1];
    float R2 = parametre[2];
    float C2 = parametre[3];

    float T1_prev = *T1;
    float T2_prev = *T2;

    float T1_inst = T1_prev + dt * (R1 * I * I + TAMB - T1_prev) / (R1 * C1);
    float T2_inst = T2_prev + dt * (T1_prev - T2_prev) / (R2 * C2);

    *T1 = T1_inst;
    *T2 = T2_inst;
}

static float modele_tension_1RC_step(float        I,
                                     float        SOC,
                                     float       *Ir,   // in/out
                                     int          etat, // 1 = décharge, 0 = charge
                                     const float *X_OCV_dep,
                                     const float *Y_OCV_dep_charge,
                                     const float *Y_OCV_dep_decharge,
                                     int          n_OCV,
                                     float        dt,
                                     float        R1,
                                     float        C1,
                                     float        R0)
{
    float denom = R1 * C1;
    float alpha = 1.0f;
    float beta  = 0.0f;

    if (denom != 0.0f)
    {
        alpha = -dt / denom + 1.0f;
        beta  =  dt / denom;
    }

    // Mise à jour Ir
    *Ir = alpha * (*Ir) + beta * I;
    //printf("Ir=%f\n", *Ir);
    
    // Choix table OCV
    const float *Y_tab = etat ? Y_OCV_dep_decharge : Y_OCV_dep_charge;

    // Interpolation OCV(SOC)
    float OCV, der_dummy;
    interp1rapide_der(X_OCV_dep, Y_tab, n_OCV, SOC, &OCV, &der_dummy);
    //OCV = 0;
    //printf("OCV=%f\n", OCV);
    //printf("der=%f\n", der_dummy);
    
    // Tension
    float U = OCV - R1 * (*Ir) - R0 * I;
    // printf("Ir=%f\n", *Ir);
    // printf("I=%f\n", I);
    return U;
}
/* ========================================================================== */
/*  Modèle SOC comptage coulombmétrique (modele_SOC_CC.m)                    */
/* ========================================================================== */
/* SOC_predit(i+1) = SOC_predit(i) - moins_eta_sur_Q*dt*I(i)/SOH             */
/* ========================================================================== */
// static void modele_SOC_CC(float moins_eta_sur_Q,
//                           float dt,
//                           int   horizon,
//                           float SOC_init,
//                           const float courant_candidat_racine,
//                           float SOH,
//                           float *SOC_predit, /*Debut paramètres tension*/
//                         ){
//     SOC_predit[0] = SOC_init;
//     SOC_predit[1] = SOC_init;
//     float SOC_inst;
//     float SOC_inst_prev = SOC_init;

//     for (int i = 0; i < horizon - 1; ++i)
//     {
//         SOC_inst =
//             SOC_inst_prev - moins_eta_sur_Q * dt * courant_candidat_racine / SOH;

//         if (SOC_inst < SOC_predit[0]){
//             SOC_predit[0] = SOC_init;
//         } 
//         if (SOC_inst > SOC_predit[1]){
//             SOC_predit[1] = SOC_init;
//         } 


//     }
// }

// /* ========================================================================== */
// /*  Modèle thermique Foster d’ordre 2 (modele_thermique_foster_ordre_2.m)    */
// /* ========================================================================== */
// static void modele_thermique_foster_ordre_2(const float parametre[4],
//                                             float       T1_init,
//                                             float       T2_init,
//                                             const float I,
//                                             float       dt,
//                                             float       TAMB,
//                                             int         horizon,
//                                             float      *T1,
//                                             float      *T2){
//     float R1 = parametre[0];
//     float C1 = parametre[1];
//     float R2 = parametre[2];
//     float C2 = parametre[3];

//     T1[0] = T1_init;
//     T2[0] = T2_init;
//     T1[1] = T1_init;
//     T2[1] = T2_init;
//     float T1_inst;
//     float T2_inst;
//     float T1_prev;
//     float T2_prev;

//     for (int i = 0; i < horizon - 1; ++i)
//     {
//         T1_inst = T1_prev
//                   + dt * (R1 * I * I + TAMB - T1_prev) / (R1 * C1);
//         T2_inst = T2_prev
//                   + dt * (T1_prev - T2_prev) / (R2 * C2);
        
//         if (T1_inst < T1[0]){
//             T1[0] = T1_inst;
//         } 
//         if (T1_inst > T1[1]){
//             T1[1] = T1_inst;
//         } 
//         if (T2_inst < T2[0]){
//             T2[0] = T2_inst;
//         } 
//         if (T2_inst > T2[1]){
//             T2[1] = T2_inst;
//         } 
//     }
// }

// /* ========================================================================== */
// /*  Modèle tension 1RC (modele_tension_1RC.m)                                */
// /* ========================================================================== */
// /* U_pred(i) = OCV(SOC(i)) - R1*Ir - R0*I(i)                                 */
// /* avec Ir(k+1) = (-dt/(R1*C1)+1)*Ir(k) + (dt/(R1*C1))*I(k)                   */
// /* ========================================================================== */
// static void modele_tension_1RC(const float courant,
//                                const float *SOC,
//                                float        Ir_init,
//                                int          etat, /* 1 = décharge, 0 = charge */
//                                const float *X_OCV_dep,
//                                const float *Y_OCV_dep_charge,
//                                const float *Y_OCV_dep_decharge,
//                                int          n_OCV,
//                                float        dt,
//                                float        R1,
//                                float        C1,
//                                float        R0,
//                                int          horizon,
//                                float       *U_pred,
//                                float       *Ir_final){
//     float Ir = Ir_init;
//     float denom = R1 * C1;
//     float alpha = 0.0f, beta = 0.0f;

//     if (denom != 0.0f)
//     {
//         alpha = -dt / denom + 1.0f;
//         beta  =  dt / denom;
//     }

//     for (int i = 0; i < horizon; ++i)
//     {
//         /* Mise à jour du courant dans le RC */
//         Ir = alpha * Ir + beta * courant[i];

//         /* Choix de la table OCV en charge / décharge */
//         const float *Y_tab = etat ? Y_OCV_dep_decharge : Y_OCV_dep_charge;

//         /* Interpolation OCV(SOC) */
//         float OCV, der_dummy;
//         interp1rapide_der(X_OCV_dep, Y_tab, n_OCV, SOC[i], &OCV, &der_dummy);

//         U_pred[i] = OCV - R1 * Ir - R0 * courant[i];
//     }

//     if (Ir_final) *Ir_final = Ir;
// }

/* ========================================================================== */
/*  Recherche de racine pour le courant (recherche_racine_SOP_Pegase_1RC.m)  */
/* ========================================================================== */
/*
 * ATTENTION :
 * ------------
 * L’algorithme MATLAB utilise :
 *  - un vecteur de résidus [SOC, T, U] en charge/décharge
 *  - des bornes A/B sur le courant
 *  - un schéma de type sécante / "pas_vecteur"
 *  - une boucle for i=1:12
 *
 * La fin de la fonction est tronquée dans l’extrait, donc je vous fournis
 * ici une structure 100% compatible avec le script, que vous pourrez
 * compléter en recopiant chaque ligne MATLAB en C (test de signe des résidus,
 * mise à jour des bornes, etc.).
 *
 * L’interface est cohérente avec script_SOP_predictif_1_RC_livraison.m.
 */
static float recherche_racine_SOP_Pegase_1RC(
    float moins_eta_sur_Q,
    float dt,
    int   horizon,
    float SOC_actuel,
    float SOH_actuel,
    const float *coefficients_modele_temperature, /* taille 4 */
    float T1_init,
    float temperature_actuelle,
    float TAMB,
    float SOC_min,
    float SOC_max,
    float U_min,
    float U_max,
    float T_max,
    float I_min,
    float I_max,
    float consigne_courant,      /* courant(i) dans le script */
    float Ir_init,
    int   etat,
    const float *X_OCV_dep,
    const float *Y_OCV_dep_charge,
    const float *Y_OCV_dep_decharge,
    int   n_OCV,
    float R1,
    float C1,
    float R0,
    float courant_requete, 
    float *residus        /* courant(i) dans le script */
){
    /* --- Allocation locale pour les prédictions --- */
    float courant_candidat_racine;
    float SOC_minmax_A[2];
    float T1_minmax_A[2], T2_minmax_A[2];
    float U_minmax_A[2];

    float SOC_minmax_B[2];
    float T1_minmax_B[2], T2_minmax_B[2];
    float U_minmax_B[2];
    float courant_final;

    bool coteA = false;
    bool coteB = false;
    // if (horizon > 64) {
    //     /* sécurité : à adapter si vous changez l’horizon */
    //     fprintf(stderr, "Horizon trop grand dans recherche_racine_SOP_Pegase_1RC\n");
    //     return consigne_courant;
    // }

    /* ---------------- Détermination des bornes A/B ---------------- */
    float borne_A, borne_B;
    if (consigne_courant > 0.0f) {
        borne_A = 0.0f;
        borne_B = I_max;
    } else {
        borne_A = 0.0f;
        borne_B = I_min;
    }

    /* ---------------- Évaluation initiale de la borne A ---------------- */
    for (int i = 0; i < horizon; ++i)
        courant_candidat_racine = borne_A;

    // modele_SOC_CC(moins_eta_sur_Q, dt, horizon,
    //               SOC_actuel, courant_candidat_racine, SOH_actuel,
    //               SOC_predit);

    // modele_thermique_foster_ordre_2(coefficients_modele_temperature,
    //                                 T1_init, temperature_actuelle,
    //                                 courant_candidat_racine, dt, TAMB,
    //                                 horizon, T1_vec, T2_vec);

    // modele_tension_1RC(courant_candidat_racine, SOC_predit,
    //                    Ir_init, etat,
    //                    X_OCV_dep, Y_OCV_dep_charge, Y_OCV_dep_decharge,
    //                    n_OCV,
    //                    dt, R1, C1, R0, horizon,
    //                    U_pred, NULL);

    //printf("SOC_actuel=%f\n", SOC_actuel);

    simuler_horizon_batterie(moins_eta_sur_Q,
                              dt,
                              horizon,
                              SOC_actuel,
                              SOH_actuel,
                              coefficients_modele_temperature,
                              T1_init,
                              temperature_actuelle,
                              TAMB,
                              Ir_init,
                              etat,
                              X_OCV_dep,
                              Y_OCV_dep_charge,
                              Y_OCV_dep_decharge,
                              n_OCV,
                              R1,
                              C1,
                              R0,
                              courant_candidat_racine,
                              SOC_minmax_A, // [min, max]
                              T1_minmax_A,
                              T2_minmax_A,
                              U_minmax_A,
                              NULL);
    
    //printf("SOC_actuel=%f\n", SOC_actuel);
    //printf("courant_racine=%f\n", courant_candidat_racine);                          
    //printf("SOC_min_A=%f\n", SOC_minmax_A[0]);
    //printf("SOC_max_A=%f\n", SOC_minmax_A[1]);
    // // printf("SOc_pred=%f\n", SOC_min_predit);
    // //printf("U=%f\n", U_minmax_A[0]);
    // printf("U=%f\n", U_minmax_A[1]);
    // //printf("T2=%f\n", T2_minmax_A[0]);
    // printf("T2=%f\n", T2_minmax_A[1]);

    // printf("corant_A=%f\n", courant_candidat_racine);
    //printf("courant_vrzi=%f\n", consigne_courant);
    float residus_borne_A[3];

    if (consigne_courant > 0.0f) {
        /* charge : contraintes (SOC_min, T_max, U_min) */
        float SOC_min_predit = SOC_minmax_A[0];
        float U_min_predit   = U_minmax_A[0];
        float T2_max         = T2_minmax_A[1];
        // for (int i = 1; i < horizon; ++i) {
        //     if (SOC_predit[i] < SOC_min_predit) SOC_min_predit = SOC_predit[i];
        //     if (U_pred[i]     < U_min_predit)   U_min_predit   = U_pred[i];
        //     if (T2_vec[i]     > T2_max)         T2_max         = T2_vec[i];
        // }
        residus_borne_A[0] = SOC_min - SOC_min_predit;
        residus_borne_A[1] = T2_max - T_max;
        residus_borne_A[2] = U_min - U_min_predit;
    } else {
        /* décharge : contraintes (SOC_max, T_max, U_max) */
        float SOC_max_predit = SOC_minmax_A[1];
        float U_max_predit   = U_minmax_A[1];
        float T2_max         = T2_minmax_A[0];
        // for (int i = 1; i < horizon; ++i) {
        //     if (SOC_predit[i] > SOC_max_predit) SOC_max_predit = SOC_predit[i];
        //     if (U_pred[i]     > U_max_predit)   U_max_predit   = U_pred[i];
        //     if (T2_vec[i]     > T2_max)         T2_max         = T2_vec[i];
        // }
        residus_borne_A[0] = SOC_max_predit - SOC_max;
        residus_borne_A[1] = T2_max - T_max;
        residus_borne_A[2] = U_max_predit - U_max;
    }

    /* Si déjà hors contraintes avec courant nul : courant_final = 0 */
    if (residus_borne_A[0] > 0.0f ||
        residus_borne_A[1] > 0.0f ||
        residus_borne_A[2] > 0.0f)
    {
         //printf("residus_A_0=%f\n", residus_borne_A[0]);
         //printf("residus_A_1=%f\n", residus_borne_A[1]);
         //printf("residus_A_2=%f\n", residus_borne_A[2]);
        return 0.0f;
    }


    /* ---------------- Évaluation initiale de la borne B ---------------- */
//printf("corant_A=%f\n", courant_candidat_racine);
courant_candidat_racine = borne_B;
//printf("corant_B=%f\n", courant_candidat_racine);

simuler_horizon_batterie(moins_eta_sur_Q,
                         dt,
                         horizon,
                         SOC_actuel,
                         SOH_actuel,
                         coefficients_modele_temperature,
                         T1_init,
                         temperature_actuelle,
                         TAMB,
                         Ir_init,
                         etat,
                         X_OCV_dep,
                         Y_OCV_dep_charge,
                         Y_OCV_dep_decharge,
                         n_OCV,
                         R1,
                         C1,
                         R0,
                         courant_candidat_racine,
                         SOC_minmax_B,   /* [min,max] */
                         T1_minmax_B,
                         T2_minmax_B,
                         U_minmax_B,
                         NULL);

float residus_borne_B[3];

if (consigne_courant > 0.0f) {
    /* charge : contraintes (SOC_min, T_max, U_min) */
    float SOC_min_predit = SOC_minmax_B[0];
    float U_min_predit   = U_minmax_B[0];
    float T2_max         = T2_minmax_B[1];

    residus_borne_B[0] = SOC_min - SOC_min_predit;
    residus_borne_B[1] = T2_max - T_max;
    residus_borne_B[2] = U_min - U_min_predit;
}
else {
    /* décharge : contraintes (SOC_max, T_max, U_max) */
    float SOC_max_predit = SOC_minmax_B[1];
    float U_max_predit   = U_minmax_B[1];
    float T2_max         = T2_minmax_B[0];

    residus_borne_B[0] = SOC_max_predit - SOC_max;
    residus_borne_B[1] = T2_max - T_max;
    residus_borne_B[2] = U_max_predit - U_max;
}

/* Si le courant max respecte toutes les contraintes → pas de limitation */
if (residus_borne_B[0] < 0.0f &&
    residus_borne_B[1] < 0.0f &&
    residus_borne_B[2] < 0.0f)
{
    //printf("residus_B_0, on s'est fait avoir=\n");
    return borne_B;
}
//printf("residus_B_0, ou pas\n");
for (int iter = 0; iter < 12; ++iter)
{
    float borne_C;
    //printf("Iteration=%d\n", iter);
    /* ----------- Calcul du pas après la première itération ----------- */
    if (iter > 0)
    {
        float pente[3];
        float pas_vecteur[3];

        for (int k = 0; k < 3; ++k)
        {
            pente[k] = (residus_borne_B[k] - residus_borne_A[k]) / (borne_B - borne_A);
            pas_vecteur[k] = -residus_borne_A[k] / pente[k];
        }

        /* Si les deux bornes satisfont déjà la contrainte → utiliser largeur de l’intervalle */
        for (int k = 0; k < 3; ++k)
        {
            if (residus_borne_A[k] < 0.0f && residus_borne_B[k] < 0.0f)
                pas_vecteur[k] = borne_B - borne_A;
        }

        /* Sélection du pas avec plus petit |pas| */
        float pas = pas_vecteur[0];
        float absmin = fabsf(pas_vecteur[0]);
        for (int k = 1; k < 3; ++k)
        {
            float a = fabsf(pas_vecteur[k]);
            if (a < absmin)
            {
                absmin = a;
                pas = pas_vecteur[k];
            }
        }

        /* Calcul de borne_C en respectant I_min / I_max */
        if (consigne_courant <= 0.0f)
            borne_C = fmaxf(borne_A + pas, I_min);
        else
            borne_C = fminf(borne_A + pas, I_max);
    }
    else
    {
        /* ----------- Première itération : borne C = courant_requete ----------- */
        //printf("première itération\n");
        borne_C = courant_requete;
    }

    /* ---------------------- Évaluation de la borne C ---------------------- */

    float SOC_minmax_C[2], T1_minmax_C[2], T2_minmax_C[2], U_minmax_C[2];

    simuler_horizon_batterie(moins_eta_sur_Q,
                             dt,
                             horizon,
                             SOC_actuel,
                             SOH_actuel,
                             coefficients_modele_temperature,
                             T1_init,
                             temperature_actuelle,
                             TAMB,
                             Ir_init,
                             etat,
                             X_OCV_dep,
                             Y_OCV_dep_charge,
                             Y_OCV_dep_decharge,
                             n_OCV,
                             R1,
                             C1,
                             R0,
                             borne_C,
                             SOC_minmax_C,
                             T1_minmax_C,
                             T2_minmax_C,
                             U_minmax_C,
                             NULL);

    float residus_borne_C[3];
    //printf("borne_C=%f\n", borne_C);
    if (consigne_courant > 0.0f) {
        /* CHARGE */
        residus_borne_C[0] = SOC_min - SOC_minmax_C[0];
        residus_borne_C[1] = T2_minmax_C[1] - T_max;
        residus_borne_C[2] = U_min - U_minmax_C[0];
    } else {
        /* DÉCHARGE */
        residus_borne_C[0] = SOC_minmax_C[1] - SOC_max;
        residus_borne_C[1] = T2_minmax_C[1] - T_max;
        residus_borne_C[2] = U_minmax_C[1] - U_max;
    }

    /* ---------------------- Mise à jour des bornes ---------------------- */
    bool violation = (residus_borne_C[0] > 0.0f ||
                      residus_borne_C[1] > 0.0f ||
                      residus_borne_C[2] > 0.0f);

    if (violation)
    {
        //printf("La borne viole les contraintes\n");
        /* --- La nouvelle borne viole les contraintes : elle devient borne_B --- */
        borne_B = borne_C;

        for (int k = 0; k < 3; ++k)
            residus_borne_B[k] = residus_borne_C[k];

        courant_final = borne_C;
        for (int k = 0; k < 3; ++k)
            residus[k] = residus_borne_C[k];

        if (coteA)
        {
            for (int k = 0; k < 3; ++k)
            {
                float gamma = residus_borne_A[k] /
                              (residus_borne_A[k] + residus_borne_C[k]);
                residus_borne_B[k] *= gamma;
            }
        }

        coteA = true;
        coteB = false;
    }
    else
    {
        //printf("La borne satisfait les contraintes\n");
           /* --- La borne satisfait les contraintes : elle devient borne_A --- */
        borne_A = borne_C;

        for (int k = 0; k < 3; ++k)
            residus_borne_A[k] = residus_borne_C[k];
            //printf("residus_borne_A_0=%f\n", residus_borne_A[0]);

        courant_final = borne_C;
        for (int k = 0; k < 3; ++k){
            //printf("k=%d\n", k);
            residus[k] = residus_borne_C[k];}
            //printf("residus_0=%f\n", residus[0]);

        if (coteB)
        {
            //printf("Mise à jour gamma coteB=true\n");
            for (int k = 0; k < 3; ++k)
            {
                float gamma = residus_borne_B[k] /
                              (residus_borne_B[k] + residus_borne_C[k]);
                residus_borne_A[k] *= gamma;
            }
        }

        coteB = true;
        coteA = false;
        //printf("coteB=true\n");
    }
}
//printf("courant_final=\n");

/* ----------------------- Saturation finale du courant ----------------------- */

if (consigne_courant <= 0.0f)
    courant_final = fmaxf(consigne_courant, courant_final);
else
    courant_final = fminf(consigne_courant, courant_final);

//printf("courant_final=%f\n", courant_final);
return courant_final;

}

/* ========================================================================== */
/*  Fonction principale : SOP_predictif (script_SOP_predictif_1_RC...)        */
/* ========================================================================== */
void SOP_predictif(
    const float *courant,
    const float *tension,
    const float *temperature,
    const float *SOH,
    const float *SOC,
    size_t       N,

    float moins_eta_sur_Q,
    const float coeffs_thermique[4],
    float TAMB,
    float T1_init,
    float T2_init,

    const float *X_OCV,
    const float *Y_OCV_charge,
    const float *Y_OCV_decharge,
    int          n_OCV,
    float R0,
    float R1,
    float C1_RC,

    float SOC_min,
    float SOC_max,
    float U_min,
    float U_max,
    float T_max,
    float I_min,
    float I_max,

    float *SOP_charge,
    float *SOP_decharge
)
{
    const float dt = 1.0f;         /* comme dans le script */

    /* --- Paramètres de réglage des correcteurs instantanés --- */
    const float Ki_T_decharge = 1.0f;
    const float Kp_T_decharge = 5.0f;
    const float Ki_T_charge   = -1.0f;
    const float Kp_T_charge   = -5.0f;
    const float Ki_Umax       = -5.0f;
    const float Kp_Umax       = -5.0f;
    const float Ki_Umin       = -5.0f;
    const float Kp_Umin       = -5.0f;

    const int horizon = 30;       /* horizon de prédiction */
    float residus[3];
    bool predictif = true;

    /* --- Buffers internes (comme dans le script) --- */
    float *courant_resultat          = (float*)calloc(N, sizeof(float));
    float *courant_candidat          = (float*)calloc(N, sizeof(float));
    float *delta_I_SOCmax            = (float*)calloc(N, sizeof(float));
    float *delta_I_Umax              = (float*)calloc(N, sizeof(float));
    float *delta_I_SOCmin            = (float*)calloc(N, sizeof(float));
    float *delta_I_Umin              = (float*)calloc(N, sizeof(float));
    float *delta_I_Tmax_charge       = (float*)calloc(N, sizeof(float));
    float *delta_I_Tmax_decharge     = (float*)calloc(N, sizeof(float));
    float *delta_I_Imax              = (float*)calloc(N, sizeof(float));
    float *delta_I_Imin              = (float*)calloc(N, sizeof(float));
    float *delta_I_consigne          = (float*)calloc(N, sizeof(float));
    float *delta_I_candidat          = (float*)calloc(N, sizeof(float));
    float *SOC_actuel                = (float*)calloc(N, sizeof(float));
    float *temperature_actuelle      = (float*)calloc(N, sizeof(float));
    float *tension_actuelle          = (float*)calloc(N, sizeof(float));
    float *Ir                        = (float*)calloc(N, sizeof(float));
    int   *etat                      = (int*)  calloc(N, sizeof(int));
    float *tampon_charge_decharge    = (float*)calloc(60, sizeof(float));
    float *moyenne_charge_decharge   = (float*)calloc(N, sizeof(float));
    float *indicateur_boucle         = (float*)calloc(N, sizeof(float));

    if (!courant_resultat || !courant_candidat || !delta_I_SOCmax || !delta_I_Umax ||
        !delta_I_SOCmin || !delta_I_Umin || !delta_I_Tmax_charge || !delta_I_Tmax_decharge ||
        !delta_I_Imax || !delta_I_Imin || !delta_I_consigne || !delta_I_candidat ||
        !SOC_actuel || !temperature_actuelle || !tension_actuelle || !Ir ||
        !etat || !tampon_charge_decharge || !moyenne_charge_decharge || !indicateur_boucle)
    {
        fprintf(stderr, "Erreur allocation mémoire dans SOP_predictif\n");
        goto cleanup;
    }

    /* Conditions initiales (copiées du script) */
    SOC_actuel[0] = SOC[0];
    SOC_actuel[1] = SOC[0];
    SOC_actuel[2] = SOC[0];

    temperature_actuelle[0] = temperature[0];
    temperature_actuelle[1] = temperature[0];
    temperature_actuelle[2] = temperature[0];

    tension_actuelle[0] = tension[0];
    tension_actuelle[1] = tension[0];
    tension_actuelle[2] = tension[0];

    Ir[0] = 0.0f; Ir[1] = 0.0f; Ir[2] = 0.0f;

    float courant_final_km1 = -courant[0];

    /* Boucle principale i = 3 : L-1 => indices [2 .. N-2] en C */
    for (size_t i = 2; i < N - 1; ++i)
    {
        /* ================================================================== */
        /* 1) Indicateur charge/décharge                                     */
        /* ================================================================== */
        /* Filtre moyenneur sur le courant (tampon de taille 60) */
        // On fait de la place pour la nouvelle valeure de courant
        for (int k = 59; k > 0; --k)
            tampon_charge_decharge[k] = tampon_charge_decharge[k - 1];
        tampon_charge_decharge[0] = -courant[i];

        /* moyenne */
        float somme = 0.0f;
        for (int k = 0; k < 60; ++k) somme += tampon_charge_decharge[k];
        moyenne_charge_decharge[i] = somme / 60.0f;

        /* Mise à jour de l’état : 0 charge, 1 décharge */
        if (i > 0)
        {
            if (moyenne_charge_decharge[i] > 0.1f && etat[i - 1] == 0)
                etat[i] = 1;
            else if (moyenne_charge_decharge[i] < -1.0f && etat[i - 1] == 1)
                etat[i] = 0;
            else
                etat[i] = etat[i - 1];
        }

        /* ================================================================== */
        /* 2) Limitation PREDICTIVE (appel à la racine SOP)                   */
        /* ================================================================== */

        float courant_final = recherche_racine_SOP_Pegase_1RC(
            moins_eta_sur_Q, dt, horizon,
            SOC_actuel[i], SOH[i],
            coeffs_thermique,
            T1_init, temperature_actuelle[i], TAMB,
            SOC_min, SOC_max, U_min, U_max, T_max,
            I_min, I_max,
            -courant[i],             /* consigne_courant */
            Ir[i], etat[i],
            X_OCV, Y_OCV_charge, Y_OCV_decharge, n_OCV,
            R1, C1_RC, R0,
            -courant[i], residus              /* courant_requete */
        );

        printf("courant_final=%f\n", courant_final);
        printf("boucle=%zu\n", i);

        /* On ne dépasse pas la consigne */
        if (-courant[i] > 0.0f)
        {
            if (courant_final > -courant[i])
                courant_final = -courant[i];
        }
        else
        {
            if (courant_final < -courant[i])
                courant_final = -courant[i];
        }

        courant_resultat[i] = courant_final;

        /* ================================================================== */
        /* 3) Limitation INSTANTANÉE (boucles SOC / U / T)                    */
        /* ================================================================== */

        /* ----------------- SOC max ----------------- */
        //float delta_I_SOCmax;
        if (SOC_actuel[i] >= SOC_max) {
            /* On impose I = 0 */
            delta_I_SOCmax[i] = -courant_candidat[i - 1];
        } else {
            /* On limite vers I_min */
            delta_I_SOCmax[i] = I_min - courant_candidat[i - 1];
        }

        /* ----------------- U max ----------------- */

        /* erreur Umax */
        float erreur_Umax = U_max - tension_actuelle[i - 1];

        /* dérivée de la tension : -(U(k)-U(k-1))/dt */
        float der_erreur_Umax =
            -(tension_actuelle[i - 1] - tension_actuelle[i - 2]) / dt;

        /* correcteur PI sur delta_I */
        delta_I_Umax[i] =
            Ki_Umax * erreur_Umax + Kp_Umax * der_erreur_Umax;

        /* saturation : delta_I ne peut pas rendre I positif */
        float max_delta_for_zero = -courant_candidat[i - 1];
        if (delta_I_Umax[i] > max_delta_for_zero)
            delta_I_Umax[i] = max_delta_for_zero;

        /* ===================== BOUCLES SOP DÉCHARGE ===================== */

        /* ---------- SOCmin ---------- */
        if (SOC_actuel[i] <= SOC_min) {
            /* On impose I = 0 si SOC est en dessous du minimum */
            delta_I_SOCmin[i] = -courant_candidat[i - 1];
        } else {
            /* Sinon, on autorise jusqu'à I_max */
            delta_I_SOCmin[i] = I_max - courant_candidat[i - 1];
        }

        /* ---------- Umin ---------- */
        float erreur_Umin = U_min - tension_actuelle[i - 1];
        float der_erreur_Umin =
            -(tension_actuelle[i - 1] - tension_actuelle[i - 2]) / dt;

        delta_I_Umin[i] =
            Ki_Umin * erreur_Umin + Kp_Umin * der_erreur_Umin;

        /* Saturation : on ne doit pas dépasser ce qui mettrait I à 0 */
        {
            float min_delta = -courant_candidat[i - 1];
            if (delta_I_Umin[i] < min_delta)
                delta_I_Umin[i] = min_delta;
        }

        /* ---------- Tmax ---------- */
        float erreur_Tmax = T_max - temperature_actuelle[i];
        float der_erreur_Tmax =
            -(temperature_actuelle[i] - temperature_actuelle[i - 1]) / dt;

        delta_I_Tmax_charge[i] =
            Ki_T_charge * erreur_Tmax + Kp_T_charge * der_erreur_Tmax;

        delta_I_Tmax_decharge[i] =
            Ki_T_decharge * erreur_Tmax + Kp_T_decharge * der_erreur_Tmax;

        /* Saturations charge / décharge */
        {
            float max_delta_charge = -courant_candidat[i - 1];
            if (delta_I_Tmax_charge[i] > max_delta_charge)
                delta_I_Tmax_charge[i] = max_delta_charge;

            float min_delta_decharge = -courant_candidat[i - 1];
            if (delta_I_Tmax_decharge[i] < min_delta_decharge)
                delta_I_Tmax_decharge[i] = min_delta_decharge;
        }

        /* ----- CALCUL DU SOP ----- */

        /* Limites courants en charge */
        float I_lim_Imin       = courant_candidat[i - 1] + delta_I_Imin[i];
        float I_lim_SOCmax     = courant_candidat[i - 1] + delta_I_SOCmax[i];
        float I_lim_Umax       = courant_candidat[i - 1] + delta_I_Umax[i];
        float I_lim_Tmax_charge= courant_candidat[i - 1] + delta_I_Tmax_charge[i];

        /* Côté charge : on prend la limite la plus "haute" (max) */
        float LIMITE_CRITIQUE_COURANTS_CHARGE = I_lim_Imin;
        if (I_lim_SOCmax      > LIMITE_CRITIQUE_COURANTS_CHARGE) LIMITE_CRITIQUE_COURANTS_CHARGE = I_lim_SOCmax;
        if (I_lim_Umax        > LIMITE_CRITIQUE_COURANTS_CHARGE) LIMITE_CRITIQUE_COURANTS_CHARGE = I_lim_Umax;
        if (I_lim_Tmax_charge > LIMITE_CRITIQUE_COURANTS_CHARGE) LIMITE_CRITIQUE_COURANTS_CHARGE = I_lim_Tmax_charge;

        /* Limites courants en décharge */
        float I_lim_Imax          = courant_candidat[i - 1] + delta_I_Imax[i];
        float I_lim_SOCmin        = courant_candidat[i - 1] + delta_I_SOCmin[i];
        float I_lim_Umin          = courant_candidat[i - 1] + delta_I_Umin[i];
        float I_lim_Tmax_decharge = courant_candidat[i - 1] + delta_I_Tmax_decharge[i];

        /* Côté décharge : on prend la limite la plus "basse" (min) */
        float LIMITE_CRITIQUE_COURANTS_DECHARGE = I_lim_Imax;
        if (I_lim_SOCmin        < LIMITE_CRITIQUE_COURANTS_DECHARGE) LIMITE_CRITIQUE_COURANTS_DECHARGE = I_lim_SOCmin;
        if (I_lim_Umin          < LIMITE_CRITIQUE_COURANTS_DECHARGE) LIMITE_CRITIQUE_COURANTS_DECHARGE = I_lim_Umin;
        if (I_lim_Tmax_decharge < LIMITE_CRITIQUE_COURANTS_DECHARGE) LIMITE_CRITIQUE_COURANTS_DECHARGE = I_lim_Tmax_decharge;

        /* SOP = courant critique * tension */
        SOP_charge[i]   = LIMITE_CRITIQUE_COURANTS_CHARGE   * tension_actuelle[i - 1];
        SOP_decharge[i] = LIMITE_CRITIQUE_COURANTS_DECHARGE * tension_actuelle[i - 1];


        /* ---------- Limites Imax / Imin ---------- */
        delta_I_Imax[i] = I_max - courant_candidat[i - 1];
        delta_I_Imin[i] = I_min - courant_candidat[i - 1];

        float delta_I_consigne;

        if (predictif) {
            delta_I_consigne = courant_resultat[i] - courant_candidat[i - 1];
        } else {
            delta_I_consigne = -courant[i] - courant_candidat[i - 1];
        }

        int   indicateur = 0;
        float delta_I_candidat;

        /* Première comparaison avec SOCmax */
        if (delta_I_consigne < delta_I_SOCmax[i]) {
            indicateur      = 1;
            delta_I_candidat = delta_I_SOCmax[i];
        } else {
            indicateur      = 0;
            delta_I_candidat = delta_I_consigne;
        }

        /* Umax */
        if (delta_I_candidat < delta_I_Umax[i]) {
            indicateur       = 2;
            delta_I_candidat = delta_I_Umax[i];
        }

        /* Tmax charge */
        if (delta_I_candidat < delta_I_Tmax_charge[i]) {
            indicateur       = 3;
            delta_I_candidat = delta_I_Tmax_charge[i];
        }

        /* SOCmin */
        if (delta_I_candidat > delta_I_SOCmin[i]) {
            indicateur       = 4;
            delta_I_candidat = delta_I_SOCmin[i];
        }

        /* Umin */
        if (delta_I_candidat > delta_I_Umin[i]) {
            indicateur       = 5;
            delta_I_candidat = delta_I_Umin[i];
        }

        /* Tmax décharge */
        if (delta_I_candidat > delta_I_Tmax_decharge[i]) {
            indicateur       = 6;
            delta_I_candidat = delta_I_Tmax_decharge[i];
        }

        /* Imax */
        if (delta_I_candidat > delta_I_Imax[i]) {
            indicateur       = 7;
            delta_I_candidat = delta_I_Imax[i];
        }

        /* Imin */
        if (delta_I_candidat < delta_I_Imin[i]) {
            indicateur       = 8;
            delta_I_candidat = delta_I_Imin[i];
        }

        /* Sauvegarde de l'indicateur si vous avez un tableau */
        indicateur_boucle[i] = indicateur;

        /* Calcul du courant final autorisé */
        courant_candidat[i] = courant_candidat[i - 1] + dt * delta_I_candidat;
        //printf("I_cand=%f\n", courant_candidat[i]);  
            
        /* --- Évaluation de la robustesse : déformation du modèle --- */
        float courant_SOC         = 1.0f * courant_candidat[i];
        float courant_temperature = 1.0f * courant_candidat[i];
        float courant_tension     = 1.0f * courant_candidat[i];

        /* --- Simulation du système sur horizon_simulation pas --- */
        float SOC = SOC_actuel[i];
        float T1  = T1_init;
        float T2  = temperature_actuelle[i];

        /* Ir pour la simulation de la tension “système” (indépendant de l'Ir de l'algo) */
        float Ir_sys = Ir[i];
        float U_sys  = 0.0f;

        for (int k = 0; k < 1; ++k)
        {
            /* 1) Avancer le SOC avec courant_SOC */
            SOC = modele_SOC_CC_step(moins_eta_sur_Q,
                                    dt,
                                    SOC,
                                    courant_SOC,
                                    SOH[i]);
            //printf("k=%d\n", k);
            //printf("SOC=%f\n", SOC);
            /* 2) Avancer le thermique avec courant_temperature */
            modele_thermique_foster_ordre_2_step(coeffs_thermique,
                                                courant_temperature,
                                                dt,
                                                TAMB,
                                                &T1,
                                                &T2);

            /* 3) Avancer la tension “système” avec courant_tension et Ir_sys local */
            U_sys = modele_tension_1RC_step(courant_tension,
                                            SOC,
                                            &Ir_sys,
                                            etat[i],
                                            X_OCV,
                                            Y_OCV_charge,
                                            Y_OCV_decharge,
                                            n_OCV,
                                            dt,
                                            R1,
                                            C1_RC,
                                            R0);
        }

        /* Mise à jour des états du système simulé pour l'itération suivante */
        SOC_actuel[i + 1]         = SOC;
        temperature_actuelle[i + 1] = T2;
        T1_init                   = T1;
        tension_actuelle[i + 1]   = U_sys;

        /* --- Mise à jour de l'état Ir de l'algorithme SOP --- */
        /* On repart de Ir[i], comme en MATLAB, et on le fait évoluer avec le courant réellement appliqué */
        Ir[i + 1] = Ir[i];
        (void) modele_tension_1RC_step(courant_candidat[i],
                                    SOC_actuel[i + 1],
                                    &Ir[i + 1],
                                    etat[i],
                                    X_OCV,
                                    Y_OCV_charge,
                                    Y_OCV_decharge,
                                    n_OCV,
                                    dt,
                                    R1,
                                    C1_RC,
                                    R0);

        printf("La valeur est : %d\n", etat[i]);

    }

Ecriture_result(courant_resultat, 10000, "courant_resultat_PC_result");
Ecriture_result(SOC_actuel, 10000, "SOP_PC_result");
Ecriture_result(tension_actuelle, 10000, "TENSION_PC_result");
Ecriture_result(temperature_actuelle, 10000, "TEMPERATURE_PC_result");
Ecriture_result_int(etat, 10000, "Etat_C");




cleanup:
    free(courant_resultat);
    free(courant_candidat);
    free(delta_I_SOCmax);
    free(delta_I_Umax);
    free(delta_I_SOCmin);
    free(delta_I_Umin);
    free(delta_I_Tmax_charge);
    free(delta_I_Tmax_decharge);
    free(delta_I_Imax);
    free(delta_I_Imin);
    free(delta_I_consigne);
    free(delta_I_candidat);
    free(SOC_actuel);
    free(temperature_actuelle);
    free(tension_actuelle);
    free(Ir);
    free(etat);
    free(tampon_charge_decharge);
    free(moyenne_charge_decharge);
    free(indicateur_boucle);
}

/* ========================================================================== */
/*  Programme autonome pour générer SOP_xxx.bin (comme SOE_Theo, etc.)       */
/* ========================================================================== */
void setup_SOP(void)
{
    const float *courant = NULL;
    const float *tension = NULL;
    const float *temperature = NULL;
    const float *SOH = NULL;
    const float *SOC = NULL;

    const int NbIteration = 10000;

    float *SOP_charge   = (float*)malloc(NbIteration * sizeof(float));
    float *SOP_decharge = (float*)malloc(NbIteration * sizeof(float));

    if (!SOP_charge || !SOP_decharge) {
        perror("malloc SOP");
        return;
    }

    /* Chargement des données brutes */
    Charge_donnees(&courant, &tension, &temperature, &SOH, &SOC);
    /* Paramètres : à reprendre de vos autres modules / fichiers .mat
       (mêmes valeurs que dans le script SOP + surveillance_tension / température) */
    const float moins_eta_sur_Q = 2.3003039e-4f;
    const float coeffs_therm[4] = {
        0.206124119186158f,
        50.3138901982787f,
        21.6224372540937f,
        15.8943772584241f
    };
    const float TAMB   = 25.0f;
    const float T1_init = 60.0f;
    const float T2_init = temperature[0];

    /* Vous pouvez soit :
       - redéclarer ici les tables OCV (comme dans surveillance_tension.c),
       - soit les mettre dans un header commun et les déclarer 'extern'. */
    
    int n_OCV = 21;


    const float X_OCV_global[] = {
    0.0,
    0.0200000000000000,
    0.0400000000000000,
    0.0600000000000000,
    0.0800000000000000,
    0.150000000000000,
    0.210000000000000,
    0.300000000000000,
    0.400000000000000,
    0.500000000000000,
    0.600000000000000,
    0.700000000000000,
    0.800000000000000,
    0.850000000000000,
    0.900000000000000,
    0.920000000000000,
    0.930000000000000,
    0.950000000000000,
    0.970000000000000,
    0.990000000000000,
    1.0
};
const float Y_OCV_charge_global[] = {
    2.74615136878047,
    2.94688704780013,
    3.04478208162645,
    3.11302386632185,
    3.14303727005486,
    3.18759401184296,
    3.22099078113839,
    3.24905171403132,
    3.27011839999992,
    3.28534786027309,
    3.29979224246840,
    3.31641533857934,
    3.33973986043135,
    3.35630007692910,
    3.38039228328910,
    3.39594494709583,
    3.40676702687123,
    3.44012152068810,
    3.48795254849592,
    3.57441784707680,
    3.60655075278407
};
const float Y_OCV_decharge_global[]= {
    2.74615136878047,
    2.94688704780013,
    3.04478208162645,
    3.11302386632185,
    3.14303727005486,
    3.18759401184296,
    3.22099078113839,
    3.24905171403132,
    3.27011839999992,
    3.28534786027309,
    3.29979224246840,
    3.31641533857934,
    3.33973986043135,
    3.350334634002046,
    3.360929407572744,
    3.365167317001023,
    3.367286271715163,
    3.371524181143442,
    3.375762090571721,
    3.380000000000000,
    3.60655075278407
};

    const float R0 = 0.022140255136947f;
    const float R1 = 0.018585867413143f;
    const float C1_RC = 8.252903566971308e+2f;

    const float SOC_min = 0.1f;
    const float SOC_max = 0.9f;
    const float U_min   = 2.0f;
    const float U_max   = 3.6f;
    const float T_max   = 60.0f;
    const float I_min   = -20.0f;
    const float I_max   = 20.0f;

    SOP_predictif(
        courant, tension, temperature, SOH, SOC,
        NbIteration,
        moins_eta_sur_Q,
        coeffs_therm,
        TAMB, T1_init, T2_init,
        X_OCV_global,
        Y_OCV_charge_global,
        Y_OCV_decharge_global,
        n_OCV,
        R0, R1, C1_RC,
        SOC_min, SOC_max,
        U_min, U_max,
        T_max,
        I_min, I_max,
        SOP_charge,
        SOP_decharge
    );

    /* Écriture des résultats */
    //Ecriture_result(SOP_charge,   NbIteration, "SOP_CHARGE_PC_result");
    //Ecriture_result(SOP_decharge, NbIteration, "SOP_DECHARGE_PC_result");

    Free_donnees(courant, tension, temperature, SOH, SOC);

    free(SOP_charge);
    free(SOP_decharge);

    //printf("Fin du SOP PC !\n");
}

int main(void) {
    // Votre code
    setup_SOP();
    return 0;
}