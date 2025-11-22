#include "Read_Write.h"

void Charge_donnees (const float **courant,const float **tension, const float **temperature, const float **SOH, const float **SOC) {
    char *fichiers[] = {"courant.bin", "tension.bin", "temperature.bin",  "SOH.bin", "SOC.bin"};
    //tableau avec les adresses des pointeurs
    const float **donnees[] = {courant, tension, temperature, SOH, SOC };
    int nFichiers = 5;
    
    size_t N = 4841577; // nombre d'éléments dans ton fichier

    for(int k = 0; k < nFichiers; k++) {
        //on modifie l'adresse vers laquel pointe le pointeur (dereferencement donnees)
        *donnees[k] = malloc(N * sizeof(float));
        if(*donnees[k] == NULL) {
            printf("Erreur allocation mémoire\n");
            // ici, tu pourrais libérer ce qui a déjà été alloué
            return;
        }
    }

    for(int k = 0; k < nFichiers; k++) {
    // Si besoin, tu peux construire un chemin complet
    char chemin[256];
    //snprintf(chemin, sizeof(chemin), "../donnees/%s", fichiers[k]); // dossier "donnees"
    snprintf(chemin, sizeof(chemin), "donnees/%s", fichiers[k]);
    //printf(chemin);
    
    FILE *fp;
    // Ouverture du fichier binaire
    fp = fopen(chemin, "rb"); // rb = read binary
    if(fp == NULL) {
        printf("Erreur ouverture fichier !\n");
        // Donner contient les adresses des pointeurs, je veux modifier l'adresse pointer par le pointeur, donc dereferencement
        free(*donnees[k]);
        return;
    }

    // Lecture des données dans le vecteur
    size_t nbLu = fread(*donnees[k], sizeof(float), N, fp);
    if(nbLu != N) {
        printf("Erreur lecture fichier : lu %zu éléments au lieu de %zu\n", nbLu, N);
    }

    fclose(fp);

    //printf("Premier : %f\n", (*donnees[k])[0]);
    //printf("Dernier : %f\n", (*donnees[k])[N-1]);
    }
}

void Free_donnees (const float *courant, const float *tension, const float *temperature, const float *SOH, const float *SOC) {
    free(courant);
    free(tension);
    free(temperature);
    free(SOH);
    free(SOC);
}

int Ecriture_result(float *data, const int NbIteration, const char *nom_fichier){
  char nom_fichier_ext[50];
  snprintf(nom_fichier_ext, sizeof(nom_fichier_ext), "%s.bin", nom_fichier);
  //printf("Nom du fichier :");
  //printf("%s\n", nom_fichier_ext);
  FILE *f = fopen(nom_fichier_ext, "wb");
  if (!f) {
        perror("Erreur ouverture fichier");
        return 1;
    }

    fwrite(data, sizeof(float), NbIteration, f);

    //printf("SOC[0] = %f, SOC[%zu] = %f\n", data[0], NbIteration-1, data[NbIteration-1]);

    fclose(f);

    //free(data);  

    return 0;
}

// ============================================================================
// Interpolation linéaire rapide 1D 
// ============================================================================
float interp1Drapide(const float *x_tab, const float *y_tab, int n, float x)
{
    if (n <= 0) return 0.0f;

    // Gestion des bornes
    if (x <= x_tab[0])
        return y_tab[0];
    if (x >= x_tab[n - 1])
        return y_tab[n - 1];

    // Recherche de l'intervalle contenant x
    for (int i = 0; i < n - 1; ++i) {
        if (x >= x_tab[i] && x <= x_tab[i + 1]) {
            float dx = x_tab[i + 1] - x_tab[i];
            float dy = y_tab[i + 1] - y_tab[i];
            if (dx == 0) return y_tab[i];
            float t = (x - x_tab[i]) / dx;
            return y_tab[i] + t * dy;
        }
    }

    // Sécurité
    return y_tab[n - 1];
}

// ============================================================================
// Reproduction de detection_phase_charge_decharge.m en C
//
// état = 0 --> charge
// état = 1 --> décharge
// ============================================================================
void detection_charge_decharge(
    float courant,
    float tampon_charge_decharge[60],
    int *etat,
    int *etat_precedent
)
{
    // tampon_charge_decharge(2:end) = tampon_charge_decharge(1:end-1);
    for (int k = 59; k > 0; --k) {
        tampon_charge_decharge[k] = tampon_charge_decharge[k - 1];
    }

    // tampon_charge_decharge(1) = courant;
    tampon_charge_decharge[0] = courant;

    // moyenne_charge_decharge = mean(tampon_charge_decharge);
    float somme = 0.0f;
    for (int k = 0; k < 60; ++k) {
        somme += tampon_charge_decharge[k];
    }
    float moyenne_charge_decharge = somme / 60.0f;

    int etat_loc = *etat_precedent;

    // Mise à jour de l'état charge/décharge
    // état = 0 --> charge
    // état = 1 --> décharge
    if (moyenne_charge_decharge > 0.1f && etat_loc == 0) {
        // on était en charge, on passe en décharge
        etat_loc = 1;
    } else if (moyenne_charge_decharge < -1.0f && etat_loc == 1) {
        // on était en décharge, on passe en charge
        etat_loc = 0;
    }
    // sinon : etat_loc inchangé

    *etat = etat_loc;
    *etat_precedent = etat_loc;
}