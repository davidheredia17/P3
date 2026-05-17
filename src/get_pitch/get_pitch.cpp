/// @file


#include <iostream>
#include <fstream>
#include <string.h>
#include <errno.h>
#include <algorithm> // Para std::sort (filtro de mediana) y std::abs
#include <cmath>     // Para std::abs


#include "wavfile_mono.h"
#include "pitch_analyzer.h"


#include "docopt.h"


#define FRAME_LEN   0.030 /* 30 ms. */
#define FRAME_SHIFT 0.015 /* 15 ms. */


using namespace std;
using namespace upc;


static const char USAGE[] = R"(
get_pitch - Pitch Estimator


Usage:
    get_pitch [options] <input-wav> <output-txt>
    get_pitch (-h | --help)
    get_pitch --version


Options:
    --window W       Window type: rect, hamming [default: rect]
    --min-f0 MIN     Minimum F0 in Hz [default: 50.0]
    --max-f0 MAX     Maximum F0 in Hz [default: 500.0]
    --clip TH        Center-clipping threshold fraction (0.0 to 1.0) [default: 0.0]
    --median N       Median filter window size (odd number, 0 to disable) [default: 0]
    -h, --help       Show this screen
    --version        Show the version of the project
   
Arguments:
    input-wav   Wave file with the audio signal
    output-txt  Output file: ASCII file with the result of the estimation:
                    - One line per frame with the estimated f0
                    - If considered unvoiced, f0 must be set to f0 = 0
)";


int main(int argc, const char *argv[]) {
  /**
  \DONE Modificació de docopt per afegir paràmetres
  Hem afegit diverses opcions a la cadena `USAGE` perquè `docopt` les processi:
  -# Tipus de finestra (`--window`): Per triar entre una finestre rectangular o Hamming.
  -# Límits de freqüència (`--min-f0`, `--max-f0`): Per ajustar el rang a on buscar.
  -# Preprocessat (`--clip`): Llindar per aplicar center clipping.
  -# Postprocesst (`--median`): Mida de la finestra pel filtre de mediana.
  Posteriorment, 'parsegem' els valors retornats per docopt i els convertim als tipus que toca.
  */
  std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
      {argv + 1, argv + argc},  // array of arguments, without the program name
      true,    // show help if requested
      "2.0");  // version string


  std::string input_wav = args["<input-wav>"].asString();
  std::string output_txt = args["<output-txt>"].asString();


  // Parse custom parameters from docopt
  float min_f0 = std::stof(args["--min-f0"].asString());
  float max_f0 = std::stof(args["--max-f0"].asString());
  float clip_th = std::stof(args["--clip"].asString());
  int median_n = std::stoi(args["--median"].asString());


  PitchAnalyzer::Window window_type = PitchAnalyzer::RECT;
  if (args["--window"].asString() == "hamming") {
    window_type = PitchAnalyzer::HAMMING;
  }


  // Read input sound file
  unsigned int rate;
  vector<float> x;
  if (readwav_mono(input_wav, rate, x) != 0) {
    cerr << "Error reading input file " << input_wav << " (" << strerror(errno) << ")\n";
    return -2;
  }


  int n_len = rate * FRAME_LEN;
  int n_shift = rate * FRAME_SHIFT;


  // Define analyzer using the parameters parsed from the command line
  PitchAnalyzer analyzer(n_len, rate, window_type, min_f0, max_f0);


  /**
  \DONE Preprocessament afegit: Center-Clipping
  El center-clipping ajuda a eliminar l'efecte dels formants i ressaltar la periodicitat fonamental
  abans de calcular l'autocorrelació.
  -# Busquem el valor màxim absolut del senyal d'entrada.
  -# Establim un llindar relatiu a aquest màxim (passat per l'argument `--clip`).
  -# Totes les mostres per sota d'aquest llindar en valor absolut es posen a zero.
  -# Restem el llindar a la resta per suavitzar la transició.
  */
  if (clip_th > 0.0f) {
    float max_val = 0.0f;
    for (float val : x) {
      if (std::abs(val) > max_val) max_val = std::abs(val);
    }
   
    float threshold = clip_th * max_val;
    for (float &val : x) {
      if (std::abs(val) < threshold) {
        val = 0.0f;
      } else {
        val = (val > 0) ? val - threshold : val + threshold;
      }
    }
  }
 
  // Iterate for each frame and save values in f0 vector
  vector<float>::iterator iX;
  vector<float> f0;
  for (iX = x.begin(); iX + n_len < x.end(); iX = iX + n_shift) {
    float f = analyzer(iX, iX + n_len);
    f0.push_back(f);
  }


  /**
  \DONE Postprocessament afegit: Filtre de Mediana temporal
  Utilitzat per corregir errors aïllats d'estimació o fallades locals en la detecció de sonor/sord.
  1. Es comprova que la mida demanada per la finestra (--median N) sigui senar.
  2. Per a cada mostra estimada de f0, s'agafa el seu entorn immediat (mida N).
  3. S'ordena l'entorn i s'extreu el valor de la mediana estadística.
  4. Es sobreescriu el valor de f0 amb el resultat suavitzat.
  */
  if (median_n > 0 && median_n % 2 != 0) {
    vector<float> f0_smooth = f0;
    int half_n = median_n / 2;
   
    for (size_t i = half_n; i < f0.size() - half_n; ++i) {
      std::vector<float> w(f0.begin() + i - half_n, f0.begin() + i + half_n + 1);
      std::sort(w.begin(), w.end());
      f0_smooth[i] = w[half_n];
    }
    f0 = f0_smooth;
  }


  // Write f0 contour into the output file
  ofstream os(output_txt);
  if (!os.good()) {
    cerr << "Error reading output file " << output_txt << " (" << strerror(errno) << ")\n";
    return -3;
  }


  os << 0 << '\n'; //pitch at t=0
  for (iX = f0.begin(); iX != f0.end(); ++iX)
    os << *iX << '\n';
  os << 0 << '\n';//pitch at t=Dur


  return 0;
}
