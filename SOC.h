#ifndef SOC_H
#define SOC_H

// Dimension du réseau (identique à votre code actuel)
#define SOC_TAILLE_ENTREE  3
#define SOC_TAILLE_RESEAU  20
#define SOC_TAILLE_SORTIE  1

// Contexte SOC : contient l'état du LSTM + SOC + Pk
typedef struct
{
    // États internes LSTM
    float xt[SOC_TAILLE_ENTREE];     // entrée normalisée [I, U, T]
    float ht[SOC_TAILLE_RESEAU];     // état caché
    float ct[SOC_TAILLE_RESEAU];     // état cellule

    // Gates intermédiaires
    float it[SOC_TAILLE_RESEAU];
    float ft[SOC_TAILLE_RESEAU];
    float gt[SOC_TAILLE_RESEAU];
    float ot[SOC_TAILLE_RESEAU];

    // Vecteurs intermédiaires pour les produits matrice*vecteur
    float vect_intermediaire_1[SOC_TAILLE_RESEAU];
    float vect_intermediaire_2[SOC_TAILLE_RESEAU];

    // Filtre de Kalman
    float SOC;   // SOC courant (0–1)
    float Pk;    // variance

} SOC_Context;

// Initialisation du contexte SOC (états LSTM + SOC + Pk)
void SOC_init(SOC_Context *ctx);

// Step SOC : 1 échantillon → 1 SOC mis à jour
// Entrées : courant (A), tension (V), température (°C), SOH (0–1)
// Retour : SOC estimé après correction Kalman
float SOC_step(SOC_Context *ctx,
               float courant,
               float tension,
               float temperature,
               float SOH);

#endif // SOC_RESEAU_CHARGE_DECH_H
