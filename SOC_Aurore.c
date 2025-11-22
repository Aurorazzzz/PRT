#include "SOC_Aurore.h"

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

void SOC_setup() {

  // dimension du LSTM
  const int tailleEntree = 3;
  const int tailleReseau = 12;
  const int tailleSortie = 1;

  // entrées & états
  float courant = 3;
  float tension = 3.05;
  float temperature = 35;
  float SOH = 1;
  float SOC = 0.1136;

// Aurore : taille des tableaux enlever. Inutile si la taille ne change pas et cause des erreurs

// Aurore : Pourquoi xt est initialisé ?

  float xt[] = {9.4559 , 4.2866 , -13.2461 };
  float ht[] = {6.4638 , -13.892 , -13.4068 , 11.2887 , 0.016961 , 3.7968 , -9.0133 , -1.9679 , 3.0694 , 1.6746 , 17.8808 , -6.2408 }; 
  float ct[] = {-1.4996 , 8.3222 , 9.4809 , -19.7374 , -3.9188 , -6.7671 , -0.16021 , 5.1517 , 4.4483 , 11.409 , 4.4768 , 3.1544 }; 
  const float Wi[] = {-2.7712 , 0.36717 , -0.57033 , 0.91807 , -1.2675 , -0.3939 , -0.51925 , 0.62105 , -1.0174 , -1.293 , -1.7955 , 0.25021 , -0.0087181 , -1.0578 , 0.25212 , 0.23335 , 0.15154 , -0.77148 , -0.4242 , 0.41126 , 0.91955 , -1.4694 , 0.070498 , 0.85216 , 1.2064 , -1.933 , 0.65713 , 0.19734 , 0.81872 , -0.75346 , -0.70688 , 1.2618 , -1.3385 , 1.2164 , 1.1673 , 0.65223 }; 
  const float Wf[] = {0.36184 , 0.83786 , -0.50614 , 0.85371 , 2.4265 , -0.18435 , -0.80977 , -0.35109 , 0.402 , -1.4671 , 1.382 , 0.53923 , -0.1039 , -0.86066 , -0.73359 , -0.82291 , 0.20689 , -0.26837 , 0.42498 , -0.2075 , 0.8706 , -1.8547 , 0.34026 , 0.33076 , 0.85858 , -0.49811 , -1.3479 , 0.30639 , -1.4214 , 1.5479 , -1.1116 , -0.27077 , -0.61664 , -0.31724 , 0.43966 , -0.69857 }; 
  const float Wo[] = {1.4472 , 0.055986 , 1.7876 , -1.2901 , -2.1 , -0.8204 , -2.2082 , 1.2353 , -0.19671 , 1.4361 , 0.0026715 , -0.89014 , -0.061672 , -0.45954 , 0.91075 , 1.1778 , -2.1032 , -0.012265 , 0.98551 , 0.37392 , 0.072814 , -1.2186 , 0.24518 , 0.93942 , -0.43188 , 0.33858 , 0.67524 , -0.83494 , -1.0781 , 0.78597 , 0.17968 , -0.73016 , -2.1327 , 1.1663 , -0.91633 , 0.21993 }; 
  const float Wg[] = {-1.4236 , -0.57036 , 0.40371 , -0.10963 , 0.93403 , 0.75616 , -0.10271 , -1.1383 , 2.1844 , -0.32272 , 1.9572 , -0.093783 , -0.83959 , -1.7632 , -0.29421 , -1.6606 , -0.32333 , -0.50481 , 3.0125 , 0.87259 , -1.927 , 1.5008 , 0.11792 , 0.52138 , -0.35899 , -1.5031 , -2.0544 , -0.11488 , 0.92184 , -0.26984 , 0.3589 , 0.16324 , -0.27503 , -0.012137 , -0.81467 , 0.84514 }; 
  const float Ri[] = {-0.37464 , 0.086573 , 0.24494 , -1.4867 , -1.1986 , -0.15832 , 1.7697 , 0.10686 , -0.19767 , -1.0096 , -0.74314 , 0.23325 , -1.6592 , 1.5126 , 1.1247 , -0.51864 , 0.61877 , 0.29297 , -1.9055 , -0.31246 , -0.578 , 0.85671 , 0.072031 , -0.54502 , -0.31023 , 1.7933 , 0.23463 , -0.69323 , -0.83241 , 2.4677 , -1.0767 , 1.2196 , -0.75597 , 0.32529 , -0.3564 , 1.0328 , -0.61994 , 1.1026 , -1.1386 , 0.57765 , -0.97049 , 1.1462 , 1.1949 , -0.90956 , 0.1212 , 1.2524 , -0.43169 , 0.37714 , -1.0246 , -0.32585 , 0.84209 , 1.455 , -0.45351 , -0.93384 , -0.29395 , 0.20398 , -1.4958 , 0.22016 , 0.77933 , 0.10926 , 0.505 , -0.58281 , 0.47634 , 1.0355 , -1.3027 , 1.1198 , -0.038178 , 0.98975 , -1.6462 , 0.8152 , 0.81632 , -0.68019 , 0.040473 , 0.75159 , -0.08839 , 1.0468 , 0.015975 , -0.92423 , 0.92393 , 0.3752 , 0.1929 , 1.1394 , 1.1483 , -1.7654 , -0.25473 , -0.86373 , -0.41304 , 0.67621 , 1.1107 , 0.009907 , 2.2231 , -0.62213 , -1.2898 , -0.41171 , -1.085 , -0.96357 , -0.54774 , -1.0709 , -0.54458 , 0.54659 , -0.24937 , 0.28937 , -1.3019 , 0.12055 , -0.39985 , -0.46107 , -0.13612 , -0.64563 , 1.6766 , -0.35524 , -0.024007 , 0.74999 , 0.4957 , 0.87 , -0.90899 , -0.3174 , 0.7754 , -1.329 , 0.65509 , 0.45253 , 1.173 , 0.95588 , -0.80051 , -0.72984 , 0.16204 , -0.39726 , -0.0775 , -0.59968 , 0.92215 , -0.0875 , -1.2935 , 0.6145 , -0.18906 , 0.15928 , -0.51219 , 0.8332 , 1.2118 , 0.53786 , 0.00051045 , 1.3532 , 1.5445 , 0.35545 , 1.9267 , -0.7853 }; 
  const float Rf[] = {0.96629 , -0.2861 , -0.083531 , 1.2559 , 1.4584 , 0.017089 , 1.7773 , 0.5484 , -0.10792 , 1.5421 , 1.1306 , 1.018 , 0.34999 , -0.22761 , 0.61642 , 0.15584 , -0.85508 , -0.36299 , -0.77952 , 1.7784 , -0.57716 , -1.7749 , 0.010653 , 0.17193 , 0.90182 , 0.67452 , -0.525 , 0.054933 , -0.99212 , -0.63124 , -0.75297 , 0.64109 , 0.52563 , 0.20566 , 0.12433 , -0.047474 , 2.1212 , 1.0368 , 1.0077 , 1.3986 , -0.011699 , -0.50032 , -1.0331 , 0.9467 , 1.152 , -0.34621 , 1.7794 , 1.6891 , 1.2486 , -0.14946 , 1.8291 , -1.7756 , 0.62691 , -0.86719 , 1.1638 , -0.35425 , 0.85838 , -1.159 , -0.23947 , 1.437 , -1.1232 , -0.31708 , 0.085288 , 0.32965 , 0.0014992 , -1.0401 , -0.58011 , -0.57882 , 0.94558 , 0.33583 , 0.85799 , -2.2511 , -0.83009 , 0.93359 , -0.068325 , -1.0579 , -0.81633 , 1.2654 , 0.4173 , 0.056473 , 1.5061 , 0.33224 , -0.87835 , 0.35649 , -0.12178 , 0.72328 , -0.56347 , -0.64337 , 0.11515 , -0.24146 , -1.6481 , -0.89332 , 0.29534 , 1.4112 , -0.76195 , -0.85024 , -0.64259 , 0.48818 , -0.47357 , 0.25844 , 0.20707 , -1.729 , -0.57268 , 1.1221 , 1.2041 , 0.13686 , 0.86278 , -0.29955 , -0.07895 , 3.2662 , -1.7035 , 0.89169 , -0.44456 , -0.48824 , 0.65923 , 0.77583 , -0.011846 , 0.57763 , 0.64831 , -0.63425 , 1.2281 , 0.031701 , -0.05372 , -0.83662 , -1.1205 , 1.0604 , -1.2804 , 0.9625 , 0.7895 , 0.24309 , 1.0581 , 1.6245 , -0.53137 , 0.34127 , -0.88134 , 0.55315 , 0.43536 , -0.53815 , 0.050124 , 1.2608 , -1.4036 , 1.4677 , -0.63319 , 1.2411 }; 
  const float Ro[] = {0.82818 , -1.6352 , -0.85661 , -0.3676 , -0.12448 , -0.53701 , 1.9371 , -1.6148 , 0.037599 , -0.36281 , -1.1979 , -1.4626 , 0.66742 , 1.224 , -0.38871 , 0.36563 , -0.41555 , -1.0402 , -0.1708 , 1.8401 , 0.13712 , -0.87412 , -0.56439 , -0.63554 , -0.1255 , 0.32517 , 1.0285 , 0.93744 , 0.87946 , 0.99728 , 0.30117 , -0.81893 , -1.5211 , 0.93159 , 1.0433 , -0.38547 , -0.95123 , -0.42891 , -0.23972 , 0.97424 , 0.56613 , -0.026054 , 0.32599 , 0.99214 , -0.018918 , 0.60004 , 0.846 , -0.94228 , 0.89204 , 0.011596 , -0.45162 , -2.1189 , 0.37592 , -0.657 , 0.92484 , 0.53382 , 0.16325 , 0.57136 , -0.49554 , -0.67377 , -0.12972 , 0.73132 , -0.97927 , -0.046465 , -0.27695 , 0.67775 , 0.21529 , -1.5267 , -0.72115 , 0.68511 , -0.20682 , -1.9242 , -1.0135 , -0.86802 , -1.1334 , -0.96678 , 0.3501 , -0.51083 , 0.36625 , 2.0229 , 0.4106 , 1.0094 , -0.1558 , -0.11242 , 0.25229 , 0.92816 , 0.022121 , -0.47125 , -0.29131 , 0.446 , 0.32215 , 0.5523 , -1.2126 , 0.99087 , -0.2754 , -0.51851 , 0.52359 , 0.69732 , 1.0402 , 1.7352 , 0.18608 , 1.5166 , 2.689 , 1.8201 , -0.57374 , 0.033691 , -2.4432 , 0.53498 , -1.4616 , -0.04515 , 1.2315 , -0.77753 , 0.5766 , 0.93776 , 0.87607 , 0.34264 , 0.10539 , -0.45027 , -0.42732 , 0.057408 , 1.8664 , 0.19121 , 0.56018 , -1.6838 , 0.33966 , -0.16016 , -0.87649 , 0.17956 , -0.6051 , -0.11066 , 0.30906 , 1.0658 , -2.1491 , -0.62876 , -0.30904 , 0.17703 , -0.67283 , -0.11384 , -1.2174 , -1.0193 , 0.42179 , 1.2379 , 2.4516 , 1.6207 }; 
  const float Rg[] = {0.55528 , 2.5318 , 0.1638 , -1.6068 , 2.0541 , -1.3504 , 1.676 , 1.6243 , 0.026693 , -0.1308 , 0.20392 , -0.091063 , 0.70342 , -1.2232 , 0.68257 , -0.20124 , 0.79894 , 0.79979 , -0.3251 , -0.14942 , -0.24052 , 0.6494 , -1.0478 , -0.02468 , 0.45816 , -1.0718 , -1.0864 , 1.1435 , -1.0711 , 0.59964 , 0.10115 , -1.9683 , 1.0385 , 0.59852 , -0.73537 , 0.19994 , 0.68398 , 0.24606 , 0.29749 , 0.66329 , -0.20516 , 1.1004 , -0.57669 , -1.6861 , -0.50723 , -0.1925 , -1.3566 , -1.0383 , 0.25129 , -0.050611 , -0.14331 , 0.16408 , -0.55444 , -1.4197 , -0.061528 , -0.086922 , -1.4831 , -1.9808 , 0.72859 , 0.52937 , -0.1785 , -0.73015 , 1.392 , 1.7854 , -0.29294 , -0.20364 , 0.72559 , 0.30739 , -0.10649 , 0.36294 , 0.60719 , -2.2171 , 0.50773 , 0.32699 , 0.30675 , -0.58771 , 1.1802 , 0.017913 , -1.2273 , 0.33755 , 0.24526 , 0.70395 , -0.82829 , 0.10742 , -0.3099 , 0.75299 , -0.53718 , 0.25904 , 0.3774 , 0.10035 , 0.19093 , 1.067 , 0.14579 , -0.7526 , 2.8876 , 0.43891 , -0.39437 , -1.1537 , -0.22893 , -0.87183 , 0.99157 , 0.87636 , -0.73482 , 0.33294 , -1.1181 , -0.016917 , 0.91914 , -0.040993 , -0.26974 , -0.40787 , -0.53622 , -0.78792 , 0.60345 , 0.70069 , 0.78838 , -0.073484 , -0.59694 , -0.44166 , 0.40579 , 0.36431 , -0.08813 , -1.2879 , 1.439 , -0.34434 , -0.79289 , 0.65202 , -1.9654 , 1.0478 , -0.67949 , 1.7013 , 1.9162 , -0.7637 , 0.0080293 , 0.083578 , -0.5111 , 0.64762 , 0.93082 , 0.17853 , -1.9555 , 2.0589 , 0.95505 , -0.030569 , -2.4557 , 0.54952 }; 
  const float bi[] = {-0.67298 , 0.93189 , -0.35789 , 0.2021 , 0.87625 , 0.80795 , -1.6033 , -2.3621 , -0.7017 , 1.6519 , 0.23507 , -0.15175 }; 
  const float bf[] = {0.12185 , -1.238 , 0.24413 , 1.3983 , -0.095473 , 0.38762 , -0.96631 , 1.5092 , 0.40383 , -0.42214 , -1.674 , -0.68763 }; 
  const float bo[] = {-0.15589 , 1.0382 , 0.33048 , 0.47581 , -2.0905 , -0.17403 , 0.019188 , -0.86003 , -0.022949 , -0.60232 , 0.8699 , -0.57081 }; 
  const float bg[] = {-1.0272 , -0.49256 , 0.34681 , 0.82935 , 0.15563 , 0.083714 , 0.37462 , 2.6532 , 0.33271 , 0.14077 , 1.5778 , 0.089548 }; 
  const float WFC[] = {1.3047 , -0.042644 , 0.89553 , 2.2849 , 0.066827 , 1.4946 , -1.0725 , 1.8233 , -1.2084 , -0.065392 , -0.32684 , -1.1492 };
  const float bFC = {-1.308};
  const float MOY[] = {0.10982 , -1.6547 , 1.1107 };
  const float ECART_TYPE[] = {2.8921 , 4.4502 , 5.0943 };

  // variables intermédiaires : pas d'initialisation
  float it[tailleReseau];
  float ft[tailleReseau];
  float gt[tailleReseau];
  float ot[tailleReseau];


  // Aurore : Les tableaux sont des const float, donc les pointeurs aussi (rajout du const)
  // Aurore : enlever les {} qui entouraient les pointeurs
  // définition des pointeurs
  float *pointeur_ht = &ht[0];
  float *pointeur_ct = &ct[0];
  float *pointeur_xt = &xt[0];
  float *pointeur_it = &it[0];
  float *pointeur_ft = &ft[0];
  float *pointeur_gt = &gt[0];
  float *pointeur_ot = &ot[0];
  const float *pointeur_Wi =  &Wi[0];
  const float *pointeur_Wf =  &Wf[0];
  const float *pointeur_Wg =  &Wg[0];
  const float *pointeur_Wo =  &Wo[0];
  const float *pointeur_Ri =  &Ri[0];
  const float *pointeur_Rf =  &Rf[0];
  const float *pointeur_Rg =  &Rg[0];
  const float *pointeur_Ro =  &Ro[0];
  const float *pointeur_bi =  &bi[0];
  const float *pointeur_bf =  &bf[0];
  const float *pointeur_bg = &bg[0];
  const float *pointeur_bo = &bo[0];
  const float *pointeur_WFC =  &WFC[0] ;
  const float *pointeur_bFC =  &bFC ;
  const float *pointeur_MOY =  &MOY[0] ;
  const float *pointeur_ECART_TYPE = &ECART_TYPE[0];

  // vecteurs intermédiaires
  float vect_intermediaire_1[tailleReseau];
  float *pointeur_vect_intermediaire_1 =  &vect_intermediaire_1[0] ;
  float vect_intermediaire_2[tailleReseau];
  float *pointeur_vect_intermediaire_2 =  &vect_intermediaire_2[0] ;

  // variables de sortie
  float sortie_LSTM;
  float *pointeur_sortie_LSTM =  &sortie_LSTM ;

  // filtre de Kalman
  const float dt = 1;
  const float moins_eta_sur_Q = 0.00023003;
  float Pk = 1;
  float *pointeur_Pk =  &Pk; 
  float *pointeur_SOC =  &SOC ; 
  const float Qk = 0.000001;
  const float Rk = 1;
  // Fk et Hk ne sont pas définies car elles valent 1 et sont multiplicatives

  // mesure de temps
  unsigned long tempsInitial;
  unsigned long tempsFinal;
  float bilanTempsLSTM;
  float bilanTempsFdK;
  const int NbIteration = 10;
  int z;

  // initialisation de la communication pour les résultats
  // Serial.begin(9600);
  //printf("");
  //printf("");

  // simulation
  // Aurore : Mesure du temps de simulation, a régler plus tard
  // tempsInitial = micros();
  for (z = 0; z <= NbIteration - 1; z++) {
    //printf("%f\n", *(pointeur_SOC));
    estimationSOC(courant, tension, temperature, SOH, moins_eta_sur_Q, pointeur_Pk, pointeur_SOC,
                    dt, Qk, Rk, tailleEntree, tailleReseau, tailleSortie, pointeur_sortie_LSTM,
                   pointeur_xt, pointeur_ct, pointeur_ht,
                   pointeur_it, pointeur_ft, pointeur_gt, pointeur_ot,
                   pointeur_Wi, pointeur_Wf, pointeur_Wg, pointeur_Wo,
                   pointeur_Ri, pointeur_Rf, pointeur_Rg, pointeur_Ro,
                   pointeur_bi, pointeur_bf, pointeur_bg, pointeur_bo,
                   pointeur_MOY, pointeur_ECART_TYPE, pointeur_WFC, pointeur_bFC,
                   pointeur_vect_intermediaire_1, pointeur_vect_intermediaire_2);

  
    //printf("%f\n", *(pointeur_SOC));
  }
  // Aurore : A regler plus tard
  //tempsFinal = micros();
 // bilanTempsLSTM = (tempsFinal - tempsInitial) / NbIteration;

  // affichage des résultats
  //printf("Sortie : ");
  //printf("%f\n", *(pointeur_SOC) *1000);
  //printf("Bilan (ms) : ");
  //printf(bilanTempsLSTM / 1000);
  //printf("");
}

// Aurore : fonction loop inutile
// void loop() {
//   // put your main code here, to run repeatedly:
// }

void estimationSOC(float courant, float tension, float temperature, float SOH, const float moins_eta_sur_Q, float *pointeur_Pk, float *pointeur_SOC,
                   const float dt, const float Qk, const float Rk, int tailleEntree, int tailleReseau, int tailleSortie, float *pointeur_sortie_LSTM,
                   float *pointeur_xt, float *pointeur_ct, float *pointeur_ht,
                   float *pointeur_it, float *pointeur_ft, float *pointeur_gt, float *pointeur_ot,
                   const float *pointeur_Wi,const float *pointeur_Wf,const float *pointeur_Wg,const float *pointeur_Wo,
                   const float *pointeur_Ri,const float *pointeur_Rf,const float *pointeur_Rg,const float *pointeur_Ro,
                   const float *pointeur_bi,const float *pointeur_bf,const float *pointeur_bg,const float *pointeur_bo,
                   const float *pointeur_MOY,const float *pointeur_ECART_TYPE,const float *pointeur_WFC,const float *pointeur_bFC,
                   float *pointeur_vect_intermediaire_1, float *pointeur_vect_intermediaire_2) {

float Sk;
float Kk;
  float residus;
  float prediction_LSTM;
  float SOC = *pointeur_SOC;
  SOC = SOC - moins_eta_sur_Q * dt * courant / SOH;
  *(pointeur_xt) = courant;
  *(pointeur_xt + 1) = tension;
  *(pointeur_xt + 2) = temperature;

  predictionLSTM(tailleEntree, tailleReseau, tailleSortie, pointeur_sortie_LSTM, pointeur_xt, pointeur_ct, pointeur_ht,
                 pointeur_it, pointeur_ft, pointeur_gt, pointeur_ot,
                 pointeur_Wi, pointeur_Wf, pointeur_Wg, pointeur_Wo,
                 pointeur_Ri, pointeur_Rf, pointeur_Rg, pointeur_Ro,
                 pointeur_bi, pointeur_bf, pointeur_bg, pointeur_bo,
                 pointeur_MOY, pointeur_ECART_TYPE, pointeur_WFC, pointeur_bFC,
                 pointeur_vect_intermediaire_1, pointeur_vect_intermediaire_2);
  prediction_LSTM = *pointeur_sortie_LSTM;
  prediction_LSTM = min(max(prediction_LSTM, 0), 1);
  *(pointeur_Pk) = *(pointeur_Pk) + Qk;  // on néglige Fk = 1
  residus = prediction_LSTM - SOC;
  Sk = *(pointeur_Pk) + Rk;  // on néglige Hk = 1;
  Kk = *(pointeur_Pk) / Sk;
  SOC = SOC + Kk * residus;
  SOC = min(max(SOC, 0), 1);
  *(pointeur_Pk) = (1 - Kk) * *(pointeur_Pk);
  *(pointeur_SOC) = SOC;
}

void predictionLSTM(int tailleEntree, int tailleReseau, int tailleSortie, float *pointeur_sortie_LSTM, float *pointeur_xt, float *pointeur_ct, float *pointeur_ht,
                    float *pointeur_it, float *pointeur_ft, float *pointeur_gt, float *pointeur_ot,
                    const float *pointeur_Wi,const float *pointeur_Wf,const float *pointeur_Wg,const float *pointeur_Wo,
                    const float *pointeur_Ri,const float *pointeur_Rf,const float *pointeur_Rg,const float *pointeur_Ro,
                    const float *pointeur_bi,const float *pointeur_bf,const float *pointeur_bg,const float *pointeur_bo,
                    const float *pointeur_MOY,const float *pointeur_ECART_TYPE,const float *pointeur_WFC,const float *pointeur_bFC,
                    float *pointeur_vect_intermediaire_1, float *pointeur_vect_intermediaire_2) {


  int i;

  // mise en forme de l'entrée
  for (i = 0; i <= tailleEntree - 1; i++) {
    *(pointeur_xt + i) = (*(pointeur_xt + i) - *(pointeur_MOY + i)) / *(pointeur_ECART_TYPE + i);
  }

  // calcul de it
  MatriceFoisVecteur(tailleReseau, tailleEntree, pointeur_Wi, pointeur_xt, pointeur_vect_intermediaire_1);
  MatriceFoisVecteur(tailleReseau, tailleReseau, pointeur_Ri, pointeur_ht, pointeur_vect_intermediaire_2);
  for (i = 0; i <= tailleReseau - 1; i++) {
    *(pointeur_it + i) = *(pointeur_vect_intermediaire_1 + i) + *(pointeur_vect_intermediaire_2 + i) + *(pointeur_bi + i);
  }
  sigma_g_sigmoide(tailleReseau, pointeur_it, pointeur_it);

  // calcul de ft
  MatriceFoisVecteur(tailleReseau, tailleEntree, pointeur_Wf, pointeur_xt, pointeur_vect_intermediaire_1);
  MatriceFoisVecteur(tailleReseau, tailleReseau, pointeur_Rf, pointeur_ht, pointeur_vect_intermediaire_2);
  for (i = 0; i <= tailleReseau - 1; i++) {
    *(pointeur_ft + i) = *(pointeur_vect_intermediaire_1 + i) + *(pointeur_vect_intermediaire_2 + i) + *(pointeur_bf + i);
  }
  sigma_g_sigmoide(tailleReseau, pointeur_ft, pointeur_ft);

  // calcul de gt
  MatriceFoisVecteur(tailleReseau, tailleEntree, pointeur_Wg, pointeur_xt, pointeur_vect_intermediaire_1);
  MatriceFoisVecteur(tailleReseau, tailleReseau, pointeur_Rg, pointeur_ht, pointeur_vect_intermediaire_2);
  for (i = 0; i <= tailleReseau - 1; i++) {
    *(pointeur_gt + i) = *(pointeur_vect_intermediaire_1 + i) + *(pointeur_vect_intermediaire_2 + i) + *(pointeur_bg + i);
  }
  sigma_c_tanh(tailleReseau, pointeur_gt, pointeur_gt);

  // calcul de ot
  MatriceFoisVecteur(tailleReseau, tailleEntree, pointeur_Wo, pointeur_xt, pointeur_vect_intermediaire_1);
  MatriceFoisVecteur(tailleReseau, tailleReseau, pointeur_Ro, pointeur_ht, pointeur_vect_intermediaire_2);
  for (i = 0; i <= tailleReseau - 1; i++) {
    *(pointeur_ot + i) = *(pointeur_vect_intermediaire_1 + i) + *(pointeur_vect_intermediaire_2 + i) + *(pointeur_bo + i);
  }
  sigma_g_sigmoide(tailleReseau, pointeur_ot, pointeur_ot);

  // calcul de ct
  for (i = 0; i <= tailleReseau - 1; i++) {
    *(pointeur_ct + i) = *(pointeur_ft + i) * *(pointeur_ct + i) + *(pointeur_it + i) * *(pointeur_gt + i);
  }

  // calcul de ht
  sigma_c_tanh(tailleReseau, pointeur_ct, pointeur_ht);
  for (i = 0; i <= tailleReseau - 1; i++) {
    *(pointeur_ht + i) = *(pointeur_ot + i) * *(pointeur_ht + i);
  }

  // calcul de la sortie
  MatriceFoisVecteur(tailleSortie, tailleReseau, pointeur_WFC, pointeur_ht, pointeur_vect_intermediaire_1);
  *(pointeur_sortie_LSTM) = *(pointeur_vect_intermediaire_1) + *pointeur_bFC;
}

void sigma_g_sigmoide(int x_size, float *pointeur_entree, float *pointeur_sortie_sig) {

  int i;

  for (i = 0; i <= x_size - 1; i++) {
    *(pointeur_sortie_sig + i) = 1 / (1 + exp(-*(pointeur_entree + i)));
  }
}

void sigma_c_tanh(int x_size, float *pointeur_entree, float *pointeur_sortie_tanh) {

  int i;

  for (i = 0; i <= x_size - 1; i++) {
    *(pointeur_sortie_tanh + i) = tanh(*(pointeur_entree + i));
  }
}


void MatriceFoisVecteur(int nbLignesMatrice, int tailleVecteur, const float *pointeur_matrice, float *pointeur_vecteur, float *pointeur_sortie_prodMat) {
  // définition des variables
  int i;
  int j;
  float resultat_tmp;


  for (i = 0; i <= nbLignesMatrice - 1; i++) {
    resultat_tmp = 0;
    for (j = 0; j <= tailleVecteur - 1; j++) {
      resultat_tmp += *(pointeur_matrice + j + i * tailleVecteur) * *(pointeur_vecteur + j);
    }
    *(pointeur_sortie_prodMat + i) = resultat_tmp;
  }
}

// Aurore : Rajout du main, sinon ça fonctionne pas

/*int main() {
    setup();
    printf("Fin du programme\n");
}*/