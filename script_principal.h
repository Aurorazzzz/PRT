#ifndef SCRIPT_PRINCIPAL_H
#define SCRIPT_PRINCIPAL_H

// ============================================================================
// Déclarations des fonctions "setup" utilisées dans main_bench
// ============================================================================

// Surveillance température
void TEMP_setup(void);

// Surveillance tension
void TENSION_setup(void);

// ============================================================================
// Si plus tard vous ajoutez d'autres modules, vous pourrez les déclarer ici.
// Exemple :
 void SOE_setup(void);
 void SOH_setup(void);
 void RUL_setup(void);
 void RINT_setup(void);
// void SOC_setup(void);
// void SOP_setup(void);
// ============================================================================

#endif // SCRIPT_PRINCIPAL_H
