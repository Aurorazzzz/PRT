#include <stdio.h>
#include <stdlib.h>

int main() {
    float *courant;
    float *tension;
    float *temperature;
    float *SOH;
    float *SOC;

    Charge_donnees(&courant, &tension, &temperature, &SOH, &SOC);

    printf("%f\n", courant[0]);

    Free_donnees(courant, tension, temperature, SOH, SOC);
    
    return 0;
}

void Charge_donnees (float **courant, float **tension, float **temperature, float **SOH, float **SOC) {
    char *fichiers[] = {"courant.bin", "temperature.bin", "tension.bin",  "SOH.bin", "SOC.bin"};
    float **donnees[] = { courant, tension, temperature, SOH, SOC };
    int nFichiers = 5;
    
    size_t N = 4841577; // nombre d'éléments dans ton fichier

    for(int k = 0; k < nFichiers; k++) {
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
    snprintf(chemin, sizeof(chemin), "../donnees/%s", fichiers[k]); // dossier "donnees"
    printf(chemin);
    
    FILE *fp;
    // Ouverture du fichier binaire
    fp = fopen(chemin, "rb"); // rb = read binary
    if(fp == NULL) {
        printf("Erreur ouverture fichier !\n");
        // Donner contient les adresses des pointeurs, je veux modifier l'adresse pointer par le pointeur, donc dereferencement
        free(*donnees[k]);
        return 1;
    }

    // Lecture des données dans le vecteur
    size_t nbLu = fread(*donnees[k], sizeof(float), N, fp);
    if(nbLu != N) {
        printf("Erreur lecture fichier : lu %zu éléments au lieu de %zu\n", nbLu, N);
    }

    fclose(fp);

    printf("Premier : %f\n", (*donnees[k])[0]);
    printf("Dernier : %f\n", (*donnees[k])[N-1]);
    }
}

void Free_donnees (float *courant, float *tension, float *temperature, float *SOH, float *SOC) {
    free(courant);
    free(tension);
    free(temperature);
    free(SOH);
    free(SOC);
}