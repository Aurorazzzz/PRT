#include "Read_Write.h"

void Charge_donnees (const float **courant,const float **tension, const float **temperature, const float **SOH, const float **SOC) {
    char *fichiers[] = {"courant.bin", "tension.bin", "temperature.bin",  "SOH.bin", "SOC.bin"};
    //tableau avec les adresses des pointeurs
    const float **donnees[] = {courant, tension, temperature, SOH, SOC };
    int nFichiers = 5;
    
    size_t N = 4841577; // nombre d'éléments dans le fichier

    for(int k = 0; k < nFichiers; k++) {
        //on modifie l'adresse vers laquel pointe le pointeur (dereferencement des donnees)
        *donnees[k] = malloc(N * sizeof(float));
        if(*donnees[k] == NULL) {
            printf("Erreur allocation mémoire\n");
            return;
        }
    }

    for(int k = 0; k < nFichiers; k++) {
    char chemin[256];
    //snprintf(chemin, sizeof(chemin), "../donnees/%s", fichiers[k]); // dossier "donnees"
    snprintf(chemin, sizeof(chemin), "donnees/%s", fichiers[k]);
    //printf(chemin);
    
    FILE *fp;
    // Ouverture du fichier binaire
    fp = fopen(chemin, "rb"); // rb = read binary
    if(fp == NULL) {
        printf("Erreur ouverture fichier !\n");
        // Donnees contient les adresses des pointeurs, je veux modifier l'adresse pointer par le pointeur, donc dereferencement
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
  snprintf(nom_fichier_ext, sizeof(nom_fichier_ext), "output/%s.bin", nom_fichier);
  //snprintf(nom_fichier_ext, sizeof(nom_fichier_ext), "%s.bin", nom_fichier);
  printf("Nom du fichier :");
  printf("%s\n", nom_fichier_ext);
  FILE *f = fopen(nom_fichier_ext, "wb");
  if (!f) {
        perror("Erreur ouverture fichier");
        return 1;
    }

    fwrite(data, sizeof(float), NbIteration, f);

    printf("SOC[0] = %f, SOC[%zu] = %f\n", data[0], NbIteration-1, data[NbIteration-1]);

    fclose(f);

    //free(data);  

    return 0;
}

int Ecriture_result_int(int *data, const int NbIteration, const char *nom_fichier){
  char nom_fichier_ext[50];
  //snprintf(nom_fichier_ext, sizeof(nom_fichier_ext), "output/%s.bin", nom_fichier);
  snprintf(nom_fichier_ext, sizeof(nom_fichier_ext), "%s.bin", nom_fichier);
  printf("Nom du fichier :");
  printf("%s\n", nom_fichier_ext);
  FILE *f = fopen(nom_fichier_ext, "wb");
  if (!f) {
        perror("Erreur ouverture fichier");
        return 1;
    }

    fwrite(data, sizeof(int), NbIteration, f);

    printf("SOC[0] = %d, SOC[%zu] = %d\n", data[0], NbIteration-1, data[NbIteration-1]);

    fclose(f);

    //free(data);  

    return 0;
}

