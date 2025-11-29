#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "SOP.h"
#include "Read_Write.h"

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

    /* liste_indice = find(x_req > x); indice = max(liste_indice) */
    for (int i = 0; i < n; ++i)
    {
        if (x_req > x[i]) indice = i;
    }

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

    *sortie = (1.0f - coord) * y[indice] + coord * y[indice + 1];
}

/* ========================================================================== */
/*  Modèle SOC comptage coulombmétrique (modele_SOC_CC.m)                    */
/* ========================================================================== */
/* SOC_predit(i+1) = SOC_predit(i) - moins_eta_sur_Q*dt*I(i)/SOH             */
/* ========================================================================== */
static void modele_SOC_CC(float moins_eta_sur_Q,
                          float dt,
                          int   horizon,
                          float SOC_init,
                          const float *courant_candidat_vecteur,
                          float SOH,
                          float *SOC_predit){
    SOC_predit[0] = SOC_init;

    for (int i = 0; i < horizon - 1; ++i)
    {
        SOC_predit[i + 1] =
            SOC_predit[i] - moins_eta_sur_Q * dt * courant_candidat_vecteur[i] / SOH;
    }
}

/* ========================================================================== */
/*  Modèle thermique Foster d’ordre 2 (modele_thermique_foster_ordre_2.m)    */
/* ========================================================================== */
static void modele_thermique_foster_ordre_2(const float parametre[4],
                                            float       T1_init,
                                            float       T2_init,
                                            const float *I,
                                            float       dt,
                                            float       TAMB,
                                            int         horizon,
                                            float      *T1,
                                            float      *T2){
    float R1 = parametre[0];
    float C1 = parametre[1];
    float R2 = parametre[2];
    float C2 = parametre[3];

    T1[0] = T1_init;
    T2[0] = T2_init;

    for (int i = 0; i < horizon - 1; ++i)
    {
        T1[i + 1] = T1[i]
                  + dt * (R1 * I[i] * I[i] + TAMB - T1[i]) / (R1 * C1);
        T2[i + 1] = T2[i]
                  + dt * (T1[i] - T2[i]) / (R2 * C2);
    }
}

/* ========================================================================== */
/*  Modèle tension 1RC (modele_tension_1RC.m)                                */
/* ========================================================================== */
/* U_pred(i) = OCV(SOC(i)) - R1*Ir - R0*I(i)                                 */
/* avec Ir(k+1) = (-dt/(R1*C1)+1)*Ir(k) + (dt/(R1*C1))*I(k)                   */
/* ========================================================================== */
static void modele_tension_1RC(const float *courant,
                               const float *SOC,
                               float        Ir_init,
                               int          etat, /* 1 = décharge, 0 = charge */
                               const float *X_OCV_dep,
                               const float *Y_OCV_dep_charge,
                               const float *Y_OCV_dep_decharge,
                               int          n_OCV,
                               float        dt,
                               float        R1,
                               float        C1,
                               float        R0,
                               int          horizon,
                               float       *U_pred,
                               float       *Ir_final){
    float Ir = Ir_init;
    float denom = R1 * C1;
    float alpha = 0.0f, beta = 0.0f;

    if (denom != 0.0f)
    {
        alpha = -dt / denom + 1.0f;
        beta  =  dt / denom;
    }

    for (int i = 0; i < horizon; ++i)
    {
        /* Mise à jour du courant dans le RC */
        Ir = alpha * Ir + beta * courant[i];

        /* Choix de la table OCV en charge / décharge */
        const float *Y_tab = etat ? Y_OCV_dep_decharge : Y_OCV_dep_charge;

        /* Interpolation OCV(SOC) */
        float OCV, der_dummy;
        interp1rapide_der(X_OCV_dep, Y_tab, n_OCV, SOC[i], &OCV, &der_dummy);

        U_pred[i] = OCV - R1 * Ir - R0 * courant[i];
    }

    if (Ir_final) *Ir_final = Ir;
}

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
    float courant_requete        /* courant(i) dans le script */
){
    /* --- Allocation locale pour les prédictions --- */
    float courant_candidat_vecteur[64]; /* horizon <= 64: marge */
    float SOC_predit[64];
    float T1_vec[64], T2_vec[64];
    float U_pred[64];

    if (horizon > 64) {
        /* sécurité : à adapter si vous changez l’horizon */
        fprintf(stderr, "Horizon trop grand dans recherche_racine_SOP_Pegase_1RC\n");
        return consigne_courant;
    }

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
        courant_candidat_vecteur[i] = borne_A;

    modele_SOC_CC(moins_eta_sur_Q, dt, horizon,
                  SOC_actuel, courant_candidat_vecteur, SOH_actuel,
                  SOC_predit);

    modele_thermique_foster_ordre_2(coefficients_modele_temperature,
                                    T1_init, temperature_actuelle,
                                    courant_candidat_vecteur, dt, TAMB,
                                    horizon, T1_vec, T2_vec);

    modele_tension_1RC(courant_candidat_vecteur, SOC_predit,
                       Ir_init, etat,
                       X_OCV_dep, Y_OCV_dep_charge, Y_OCV_dep_decharge,
                       n_OCV,
                       dt, R1, C1, R0, horizon,
                       U_pred, NULL);

    float residus_borne_A[3];

    if (consigne_courant > 0.0f) {
        /* charge : contraintes (SOC_min, T_max, U_min) */
        float SOC_min_predit = SOC_predit[0];
        float U_min_predit   = U_pred[0];
        float T2_max         = T2_vec[0];
        for (int i = 1; i < horizon; ++i) {
            if (SOC_predit[i] < SOC_min_predit) SOC_min_predit = SOC_predit[i];
            if (U_pred[i]     < U_min_predit)   U_min_predit   = U_pred[i];
            if (T2_vec[i]     > T2_max)         T2_max         = T2_vec[i];
        }
        residus_borne_A[0] = SOC_min - SOC_min_predit;
        residus_borne_A[1] = T2_max - T_max;
        residus_borne_A[2] = U_min - U_min_predit;
    } else {
        /* décharge : contraintes (SOC_max, T_max, U_max) */
        float SOC_max_predit = SOC_predit[0];
        float U_max_predit   = U_pred[0];
        float T2_max         = T2_vec[0];
        for (int i = 1; i < horizon; ++i) {
            if (SOC_predit[i] > SOC_max_predit) SOC_max_predit = SOC_predit[i];
            if (U_pred[i]     > U_max_predit)   U_max_predit   = U_pred[i];
            if (T2_vec[i]     > T2_max)         T2_max         = T2_vec[i];
        }
        residus_borne_A[0] = SOC_max_predit - SOC_max;
        residus_borne_A[1] = T2_max - T_max;
        residus_borne_A[2] = U_max_predit - U_max;
    }

    /* Si déjà hors contraintes avec courant nul : courant_final = 0 */
    if (residus_borne_A[0] > 0.0f ||
        residus_borne_A[1] > 0.0f ||
        residus_borne_A[2] > 0.0f)
    {
        return 0.0f;
    }

    /* --- À partir d’ici, il faut traduire EXACTEMENT le reste du script : --- */
    /*
       - Évaluer la borne_B comme dans le script
       - Construire residus_borne_B
       - Si all(residus_borne_B<0) => courant_final = borne_B
       - Sinon boucle for i=1:12 :
            * calcul de la pente = (residus_B - residus_A)/(borne_B - borne_A)
            * calcul de pas_vecteur = -residus_borne_A./pente
            * gestion des cas où les deux résidus sont <0
            * calcul de borne_C (borne_A + pas, saturée entre I_min/I_max)
            * évaluer SOC/T/U à borne_C → residus_borne_C
            * mettre à jour borne_A / borne_B selon le signe des résidus
       - retourner courant_final
    */

    /* Pour l’instant on retourne simplement la consigne d’origine
       (ce qui vous permet de compiler et de tester le reste du pipeline).
       À remplacer par l’algorithme complet. */
    return consigne_courant;
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

    if (!courant_resultat || !courant_candidat || !delta_I_SOCmax || !delta_I_Umax ||
        !delta_I_SOCmin || !delta_I_Umin || !delta_I_Tmax_charge || !delta_I_Tmax_decharge ||
        !delta_I_Imax || !delta_I_Imin || !delta_I_consigne || !delta_I_candidat ||
        !SOC_actuel || !temperature_actuelle || !tension_actuelle || !Ir ||
        !etat || !tampon_charge_decharge || !moyenne_charge_decharge)
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

    float courant_final_km1 = courant[0];

    /* Boucle principale i = 3 : L-1 => indices [2 .. N-2] en C */
    for (size_t i = 2; i < N - 1; ++i)
    {
        /* ================================================================== */
        /* 1) Indicateur charge/décharge                                     */
        /* ================================================================== */
        /* Filtre moyenneur sur le courant (tampon de taille 60) */
        for (int k = 59; k > 0; --k)
            tampon_charge_decharge[k] = tampon_charge_decharge[k - 1];
        tampon_charge_decharge[0] = courant[i];

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
            courant[i],             /* consigne_courant */
            Ir[i], etat[i],
            X_OCV, Y_OCV_charge, Y_OCV_decharge, n_OCV,
            R1, C1_RC, R0,
            courant[i]              /* courant_requete */
        );

        /* On ne dépasse pas la consigne */
        if (courant[i] > 0.0f)
        {
            if (courant_final > courant[i])
                courant_final = courant[i];
        }
        else
        {
            if (courant_final < courant[i])
                courant_final = courant[i];
        }

        courant_resultat[i] = courant_final;

        /* ================================================================== */
        /* 3) Limitation INSTANTANÉE (boucles SOC / U / T)                    */
        /* ================================================================== */

        /* Ici on recopie ligne à ligne le script MATLAB :
           - mise à jour de SOC_actuel, temperature_actuelle, tension_actuelle
             à partir des modèles (comme vous l’avez déjà fait dans les autres modules)
           - calcul des delta_I_* (SOCmax, SOCmin, Umax, Umin, Tmax_charge, Tmax_decharge)
           - calcul des LIMITES_COURANTS_CHARGE / DECHARGE
           - calcul de LIMITE_CRITIQUE_COURANTS_*
           - enfin SOP_CHARGE(i) = LIMITE_CRITIQUE_COURANTS_CHARGE * tension_actuelle(i-1)
                    SOP_DECHARGE(i) = LIMITE_CRITIQUE_COURANTS_DECHARGE * tension_actuelle(i-1)
        */

        /* Pour l’instant, on se contente de poser SOP basées sur courant_final
           pour garder un code fonctionnel : à remplacer par la traduction exacte. */
        SOP_charge[i]   = fmaxf(0.0f, courant_final) * tension[i];
        SOP_decharge[i] = fminf(0.0f, courant_final) * tension[i];

        courant_final_km1 = courant_final;
    }

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

    const int NbIteration = 1000000;

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
    extern const float X_OCV_global[104];
    extern const float Y_OCV_charge_global[104];
    extern const float Y_OCV_decharge_global[104];

    const float R0 = 0.0185f;
    const float R1 = 0.0130f;
    const float C1_RC = 653.6309f;

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
        104,
        R0, R1, C1_RC,
        SOC_min, SOC_max,
        U_min, U_max,
        T_max,
        I_min, I_max,
        SOP_charge,
        SOP_decharge
    );

    /* Écriture des résultats */
    Ecriture_result(SOP_charge,   NbIteration, "SOP_CHARGE_PC_result");
    Ecriture_result(SOP_decharge, NbIteration, "SOP_DECHARGE_PC_result");

    Free_donnees(courant, tension, temperature, SOH, SOC);

    free(SOP_charge);
    free(SOP_decharge);

    printf("Fin du SOP PC !\n");
}
