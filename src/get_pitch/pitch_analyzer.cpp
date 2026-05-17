/// @file


#include <iostream>
#include <math.h>
#include "pitch_analyzer.h"
#include <fstream>


using namespace std;


/// Name space of UPC
namespace upc {
  void PitchAnalyzer::autocorrelation(const vector<float> &x, vector<float> &r) const {


    for (unsigned int l = 0; l < r.size(); ++l) {
      /**
      \DONE Autocorrelació calculada
      \f[
      r[l] = \frac{1}{N} \sum_{n=l}^{n=N} x[n] \cdot x[n-l]
      \f]
      -# Inicialitzem \f$r[l]\f$ a zero
      -# Acumulem el producte de \f$x[n]\f$ per \f$x[n-l]\f$ per a \f$l\le n < N\f$
      -# Dividim el resultat per \f$N\f$
      */
     
      // r[l] = sum (x[n]*x[n-l])
      r[l] = 0;
      for(unsigned int n = l; n < x.size(); n++){
        r[l] += x[n] * x[n-l];
      }
      r[l] = r[l] / x.size();
    }


    if (r[0] == 0.0F) //to avoid log() and divide zero
      r[0] = 1e-10;
  }


  void PitchAnalyzer::set_window(Window win_type) {
    if (frameLen == 0)
      return;


    window.resize(frameLen);


    switch (win_type) {
    case HAMMING:
      /**
      \DONE Finestra de Hamming implementada
      \f[
      w[n] = 0.54 - 0.46 \cdot \cos\left(\frac{2\pi n}{N-1}\right)
      \f]
      On \f$N\f$ és la longitud de la trama (frameLen).
      -# Recorrem l'array de la finestra.
      -# Apliquem la fórmula matemàtica per a cada índex \f$i\f$.
      */
      for (unsigned int i = 0; i < frameLen; ++i) {
        window[i] = 0.54f - 0.46f * cos(2.0f * M_PI * i / (frameLen - 1));
      }
      break;
    case RECT:
    default:
      window.assign(frameLen, 1);
    }
  }


  void PitchAnalyzer::set_f0_range(float min_F0, float max_F0) {
    npitch_min = (unsigned int) samplingFreq/max_F0;
    if (npitch_min < 2)
      npitch_min = 2;  // samplingFreq/2


    npitch_max = 1 + (unsigned int) samplingFreq/min_F0;


    //frameLen should include at least 2*T0
    if (npitch_max > frameLen/2)
      npitch_max = frameLen/2;
  }


  bool PitchAnalyzer::unvoiced(float pot, float r1norm, float rmaxnorm) const {
    /**
    \DONE Regla de decisió sonor/sord (voiced/unvoiced) implementada
    \n Hem implementat una heurística basada en la potència i la periodicitat de la senyal:
    -# Si la potència (`pot`) és menor a -45 dB, assumim que és soroll de fons o silenci, per tant és sord fem `return true`.
    -# Si el valor màxim secundari de l'autocorrelació normalitzada (`rmaxnorm`) és menor a 0.43,
       significa que no hi ha prou periodicitat a la senyal, per tant la classifiquem com a sorda (`return true`).
    -# Si supera ambdós llindars, la considerem sonora (`return false`).
    -# A més, si la potència és bastant baixa (menor a -38 dB) i la periodicitat no és clara (rmaxnorm < 0.55), també la classifiquem com a sorda.
    Tots aquests paràmetres són empírics i segurament es poden optimitzar encara mes.
    */
   
    if (pot < -45.0f) {
      return true;
    }
   
    if (rmaxnorm < 0.43f) {
      return true;
    }
    // Si la potencia baixa i la periodicitat no es clara sord també
    if (pot < -38.0f && rmaxnorm < 0.55f) return true;


    return false;
  }


  float PitchAnalyzer::compute_pitch(vector<float> & x) const {
    if (x.size() != frameLen)
      return -1.0F;


    //Window input frame
    for (unsigned int i=0; i<x.size(); ++i)
      x[i] *= window[i];


    vector<float> r(npitch_max);


    //Compute correlation
    autocorrelation(x, r);


    vector<float>::const_iterator iR = r.begin(), iRMax = iR;


    /**
    \DONE Cerca del període de pitch (lag de la màxima autocorrelació lluny de l'origen)
    -# Per evitar detectar el màxim global al lag 0 (energia de la senyal), iniciem
       la recerca de `iRMax` a partir de `npitch_min`, que correspon a la freqüència
       màxima possible del pitch esperat (500 Hz per la veu humana segons la teoria).
    -# Recorrem l'array d'autocorrelació `r` des de `npitch_min` fins a la fi de l'array
       (que està limitat per `npitch_max`, la freqüència mínima del pitch).
    -# Actualitzem `iRMax` cada cop que trobem un valor superior a l'actual emmagatzemat.
    */


    // Comencem a buscar des de npitch_min
    iRMax = r.begin() + npitch_min;
   
    // Recorrem el vector des de npitch_min fins el final (npitch_max)
    for (vector<float>::const_iterator it = r.begin() + npitch_min; it != r.end(); it++) {
      if (*it > *iRMax) {
        iRMax = it;
      }
    }


    unsigned int lag = iRMax - r.begin();


    float pot = 10 * log10(r[0]);


    //You can print these (and other) features, look at them using wavesurfer
    //Based on that, implement a rule for unvoiced
    //change to #if 1 and compile
#if 1
    if (r[0] > 0.0F) {
      cout << pot << '\t' << r[1]/r[0] << '\t' << r[lag]/r[0] << endl;
    } else {
      // Imprimim valors molt baixos per representar els silencis absoluts
      cout << -100.0 << '\t' << 0.0 << '\t' << 0.0 << endl;
    }
#endif
   
    if (unvoiced(pot, r[1]/r[0], r[lag]/r[0]))
      return 0;
    else
      return (float) samplingFreq/(float) lag;
  }
}
