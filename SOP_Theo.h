#ifndef SOP_THEO_H
#define SOP_THEO_H

#include <stddef.h>   // pour size_t
#include <stdio.h>    // pour printf si besoin dans les logs

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Estimation du SOP (State Of Power) — traduction du script MATLAB
// Entrées :
//   inputArg1, inputArg2 : variables d’entrée (ex. courant, tension)
// Sorties :
//   *outputArg1, *outputArg2 : variables de sortie (ex. puissance dispo, etc.)
// Remarques :
//   - Cette fonction est un squelette de base, à compléter avec le vrai modèle
// ============================================================================
void estimation_SOP(float inputArg1,
                    float inputArg2,
                    float* outputArg1,
                    float* outputArg2);

// ============================================================================
// setup()
//   Initialise les buffers de données, appelle la fonction d’estimation,
//   puis libère la mémoire. Utilisé dans le main().
// ============================================================================
void setup(void);

#ifdef __cplusplus
}
#endif

#endif // SOP_THEO_H
