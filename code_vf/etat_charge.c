#include "etat_charge.h"

int etat_charge_decharge(ETAT_Context *ctx, float I_mes)
{
    int idx = ctx->idx;

    /* =========================
       Remplissage tampon (ring buffer) : PAS de décalage
       ========================= */
    if (idx < 0 || idx >= BUF_LEN) {
        idx = 0;     // sécurité
    }

    ctx->buf[idx] = I_mes;   // on écrase la plus ancienne case

    idx++;
    if (idx >= BUF_LEN) {
        idx = 0;
    }
    ctx->idx = idx;

    /* =========================
       Vote par moyenne : inchangé
       ========================= */
    float somme = 0.0f;
    for (int k = 0; k < BUF_LEN; ++k) {
        somme += ctx->buf[k];
    }
    float moyenne_charge_decharge = somme / (float)BUF_LEN;

    int etat_loc = ctx->etat;

    /* état = 0 --> charge
       état = 1 --> décharge */
    if (moyenne_charge_decharge > 0.1f && etat_loc == 0) {
        etat_loc = 1;
    } 
    else if (moyenne_charge_decharge < -1.0f && etat_loc == 1) {
        etat_loc = 0;
    }

    ctx->etat = etat_loc;
    return ctx->etat;
}

void ETAT_init(ETAT_Context *ctx)
{


    /* État initial : charge (0) ou adaptez si besoin */
    ctx->etat = 0;

    /* Index du tampon circulaire */
    ctx->idx = 0;

    /* Tampon initialisé à REPOS (= 0) */
    for (int i = 0; i < BUF_LEN; ++i) {
        ctx->buf[i] = 0;
    }
}
