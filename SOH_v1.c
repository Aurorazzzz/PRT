#include "Read_Write.h"
#include "SOH_v1.h"


void calcul_vecteur_SOH(){
    // Initialisation des entrées
    const float *courant;
    const float *tension;
    const float *temperature;
    const float *SOH_simu;
    const float *SOC;

    //Initialisation du nombre de données simulées
    const int NbIteration = 1000000;

    //Initialisation du vecteur contenant le SOC :
    float *vecteur_SOH = malloc(NbIteration * sizeof(float));
    if (!vecteur_SOH) {
        perror("Erreur allocation mémoire");
        return;
    }

    // Initialisation des coefficients du filtre 
    const double a_filtre[] = {1, -0.969067417193793 };
    const double b_filtre[] = {0.0154662914031034,	0.0154662914031034 };

    //Initialisation variables charge décharge
    const int taille_tampon = 60;

    float tampon_charge_decharge[60] = {0};
    bool etat_precedant = 0;
    bool changement_etat = 0;
    //float *pointeur_tampon =  &tampon_charge_decharge; 
    bool *pointeur_etat_precedant =  &etat_precedant ; 
    bool *pointeur_changement_etat = &changement_etat;

    //Initialisation comptage coulombimetrique
    // Aurore : peut passer en variable globale plus tard
    const float dt = 1;
    const float moins_eta_sur_Q = 0.00023003;

    //Initialisation des varibales pour le SOH
    float integrale_courant = 0;
    float *pointeur_integrale_courant = &integrale_courant;
    const float integrale_courant_neuf = 1/moins_eta_sur_Q;
    float SOC_precedent = 0;
    float *pointeur_SOC_precedent = &SOC_precedent;

    //Initialisation variables filtre
    float SOH_n_1 = 1;
    float SOH_pre_filtre_n_1 = 1;
    float *pointeur_SOH_n_1 = &SOH_n_1;
    float *pointeur_SOH_pre_filtre_n_1 = &SOH_pre_filtre_n_1;

    //Initialisation du SOH
    float SOH = 1;
    float *pointeur_SOH = &SOH;

    Charge_donnees(&courant, &tension, &temperature, &SOH_simu, &SOC);
    printf("premier courant : ");
    printf("%f\n", courant[0]);

    for (int z = 0; z <= NbIteration - 1; z++) {
        
        detection_charge_decharge(tampon_charge_decharge, taille_tampon, -courant[z], pointeur_etat_precedant, pointeur_changement_etat);
        calcul_SOH(pointeur_integrale_courant, -courant[z], changement_etat, dt, integrale_courant_neuf, pointeur_SOC_precedent, SOC[z], pointeur_SOH,
             pointeur_SOH_n_1, pointeur_SOH_pre_filtre_n_1, a_filtre, b_filtre);

        //vecteur_SOH[z] = *(pointeur_SOH);
        vecteur_SOH[z] = *(pointeur_SOH);
        printf("%f\n", *(pointeur_SOH));
    }

    Free_donnees(courant, tension, temperature, SOH_simu, SOC);
    Ecriture_result(vecteur_SOH, NbIteration, "SOH_ordi");
}

void calcul_SOH(float *integrale_courant, const float courant, bool changement_etat, const float dt, const float integrale_courant_neuf, 
    float *SOC_precedent, const float SOC, float *SOH, float *y_n_1, float *x_n_1, const double a_filtre[], const double b_filtre[]){
    *integrale_courant = *integrale_courant + courant*dt; 
    

    if (changement_etat){
        // condition : l'intégrale de courant parcourue depuis la dernière
        // estimation représente au moins 10% de la pleine charge à neuf
        bool condition_SOH = ((fabsf(*integrale_courant)/integrale_courant_neuf)>0.1f);
        //printf("Changement potentiel du SOH \n");
        if (condition_SOH) {
            // Estimation réalisée par règle de trois : (intégrale de courant
            // relevée) / ( (écart de SOC relevé)*intégrale de courant à neuf )

            float SOH_pre_filtre = *integrale_courant/((*SOC_precedent-SOC)*integrale_courant_neuf);
            //printf("Changement effectif du SOH \n");
            //printf("%f\f", *integrale_courant);
            //printf("%f\f", *SOC_precedent);
            //printf("%f\f", SOC);
            //Saturation pour eviter resultats aberrants
            if ((SOH_pre_filtre>1) || SOH_pre_filtre<0){
                //printf("%f\f", SOH_pre_filtre);
                SOH_pre_filtre = *SOH;
                 printf("Resultat aberrant \n");
                 
            }

            //filtrage du SOH : filtre discret d'ordre 1
            *SOH = (-a_filtre[1]*(*y_n_1)+b_filtre[0]*SOH_pre_filtre+b_filtre[1]*(*x_n_1))/a_filtre[0];
            *y_n_1 = *SOH;
            *x_n_1 = SOH_pre_filtre;
        
        }

        *SOC_precedent = SOC;
        //printf("Le SOC egal");
        //printf("%f\f", *SOC_precedent);
        //printf("%f\f", SOC);
        *integrale_courant = 0;

    }
}

void detection_charge_decharge(float *tampon_charge_decharge, const int taille_tampon, const float courant, bool *etat_precedent, bool *changement_etat){
    //Decalage vers la droite
    bool etat;
    memmove(&tampon_charge_decharge[1], &tampon_charge_decharge[0], (taille_tampon - 1) * sizeof(float));
    tampon_charge_decharge[0] = courant;

    //printf("courant avec f: ");
    //printf("%f\f", courant);

    float moyenne_charge_decharge = moyenne(tampon_charge_decharge, taille_tampon);
    //printf("tampon : ");
    //printf("%f\f", tampon_charge_decharge[0]);

    // Mise à jour de l'état charge/décharge
    if ((moyenne_charge_decharge > 0.1f) && !(*etat_precedent)) {
        etat = 1;  // passe en décharge
    } else if ((moyenne_charge_decharge < -1.0f) && *etat_precedent) {
        etat = 0; // passe en charge
    } else {
        etat = *etat_precedent; // pas de changement
    }
    //printf("Moyenne");
    //printf("%f\f", moyenne_charge_decharge);

    //printf("Etat");
    //printf("%d\n", *etat_precedent);

    // Détection de changement d’état
    *changement_etat = (etat != *etat_precedent);
    //printf("changemenet etat : ");
    //printf("%d\n", *changement_etat);
    // Mise à jour de l’état précédent
    *etat_precedent = etat;
}

float moyenne(float *tableau, const int taille) {

    float somme = 0;
    for (size_t i = 0; i < taille; i++) {
        somme += tableau[i];
    }
    return somme / (float)taille;
}

int main() {
    calcul_vecteur_SOH();
    printf("Fin du programme\n");
}