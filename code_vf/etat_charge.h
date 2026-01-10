#ifndef ETAT_CHARGE_H
#define ETAT_CHARGE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Taille du tampon (source unique de vérité) */
#define BUF_LEN 60

typedef struct {
    int   etat;            // état actuel (0=charge, 1=decharge)
    float buf[BUF_LEN];    // tampon circulaire de courants
    int   idx;             // index d’écriture
} ETAT_Context;

/* Initialise le contexte (etat=charge, tampon remis à zéro) */
void ETAT_init(ETAT_Context *ctx);

/*
 * Met à jour l’état charge/décharge à partir du courant.
 * Convention : I_mes > 0 => CHARGE ; I_mes < 0 => DECHARGE.
 */
int etat_charge_decharge(ETAT_Context *ctx, float I_mes);

#ifdef __cplusplus
}
#endif

#endif /* ETAT_CHARGE_H */
