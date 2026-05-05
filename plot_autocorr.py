import matplotlib.pyplot as plt
import numpy as np
import sys

# ==============================================================================
# Instruccions per generar 'frame_data.txt':
# Necessites exportar temporalment des de C++ els valors d'un frame SONOR.
# Una manera senzilla (per debug) és modificar pitch_analyzer.cpp temporalment:
# En compute_pitch, després de cridar autocorrelation(x, r):
#   ofstream out("frame_data.txt");
#   for (size_t i = 0; i < x.size(); ++i) {
#       out << x[i] << " " << r[i] << "\n"; 
#   }
# I executar el programa amb un àudio de prova només per capturar un frame.
# ==============================================================================

def plot_autocorrelation(data_file, sample_rate=16000):
    """
    Llegeix els valors de la senyal temporal i la seva autocorrelació des d'un 
    fitxer de text, i genera la gràfica demanada a l'exercici.
    """
    try:
        # Carreguem les dades. Assumim que hi ha dues columnes (espai/tab/coma)
        # Columna 0: Senyal temporal (x)
        # Columna 1: Autocorrelació (r)
        data = np.loadtxt(data_file)
        
        # Si només tens una columna temporalment i has d'extreure la autocorrelació manualment
        # caldria adaptar això. Però assumim el cas ideal on C++ ens dóna els dos.
        signal = data[:, 0]
        autocorr = data[:, 1]
        
    except Exception as e:
        print(f"Error en llegir el fitxer '{data_file}': {e}")
        print("Assegura't de generar el fitxer des de C++ amb un frame sonor (ex: 30ms)")
        # Creem dades falses només perquè l'script funcioni a mode de demostració
        # si no existeix el fitxer (IMPORTANT: Canviar a dades reals per a la memòria!)
        print("Generant dades de demostració...")
        t_demo = np.linspace(0, 0.03, 480) # 30ms a 16kHz
        f0_demo = 150 # Hz
        signal = np.sin(2 * np.pi * f0_demo * t_demo) + 0.5 * np.sin(2 * np.pi * 2 * f0_demo * t_demo)
        autocorr = np.correlate(signal, signal, mode='full')[len(signal)-1:] / len(signal)
        # Retallem l'autocorrelació simulada per que encaixi amb la mida del frame
        autocorr = autocorr[:len(signal)]

    
    # 1. Paràmetres bàsics
    N = len(signal)                 # Nombre de mostres del frame
    t_ms = np.arange(N) * 1000.0 / sample_rate # Eix temporal en mil·lisegons
    lag_ms = np.arange(N) * 1000.0 / sample_rate # Eix de lag (retard) en ms
    
    # 2. Cerca del primer màxim secundari (Càlcul equivalent al que fas en C++)
    # Límits del pitch típics per a la veu humana (ex: 50 Hz a 500 Hz)
    min_f0_hz = 50.0
    max_f0_hz = 500.0
    
    # Índexs del lag on busquem el màxim (npitch_min i npitch_max a C++)
    lag_min_idx = int(sample_rate / max_f0_hz)
    lag_max_idx = int(sample_rate / min_f0_hz)
    
    # Ens assegurem de no sortir de la finestra
    lag_max_idx = min(lag_max_idx, N - 1)
    
    # Trobem l'índex del màxim dins d'aquest rang
    autocorr_search_range = autocorr[lag_min_idx:lag_max_idx]
    max_sec_idx_relative = np.argmax(autocorr_search_range)
    max_sec_idx = lag_min_idx + max_sec_idx_relative
    
    # Període estimat
    T0_ms = lag_ms[max_sec_idx]
    F0 = sample_rate / max_sec_idx
    
    print(f"Període estimat: {T0_ms:.2f} ms")
    print(f"Freqüència Fonamental (F0): {F0:.2f} Hz")

    # 3. Creació de la figura amb 2 subplots (un a dalt, l'altre a baix)
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
    fig.subplots_adjust(hspace=0.4) # Espai entre subplots

    # --- SUBPLOT 1: Senyal Temporal ---
    ax1.plot(t_ms, signal, color='blue', linewidth=1.5)
    ax1.set_title('Senyal temporal d\'un segment de 30 ms d\'un fonema sonor', fontsize=14, fontweight='bold')
    ax1.set_xlabel('Temps (ms)', fontsize=12)
    ax1.set_ylabel('Amplitud', fontsize=12)
    ax1.grid(True, linestyle='--', alpha=0.7)
    
    # Dibuixem fletxes o línies per marcar el període a la senyal (prenent un pic com a referència)
    # Busquem el primer pic gran per tenir una referència visual (opcional per millorar la gràfica)
    pic_ref_idx = np.argmax(signal[:int(N/2)])
    ax1.axvline(x=t_ms[pic_ref_idx], color='red', linestyle='--', alpha=0.7)
    ax1.axvline(x=t_ms[pic_ref_idx] + T0_ms, color='red', linestyle='--', alpha=0.7)
    
    # Afegim una fletxa d'anotació indicant el període
    ax1.annotate(f'Període ($T_0$) $\\approx$ {T0_ms:.2f} ms', 
                 xy=(t_ms[pic_ref_idx] + T0_ms/2, np.max(signal)*0.8),
                 xytext=(t_ms[pic_ref_idx] + T0_ms/2, np.max(signal)*1.1),
                 ha='center', va='bottom',
                 arrowprops=dict(arrowstyle='<->', color='red'))

    # --- SUBPLOT 2: Autocorrelació ---
    ax2.plot(lag_ms, autocorr, color='green', linewidth=1.5)
    ax2.set_title('Autocorrelació de la senyal (Funció $r[l]$)', fontsize=14, fontweight='bold')
    ax2.set_xlabel('Retard / Lag (ms)', fontsize=12)
    ax2.set_ylabel('Autocorrelació', fontsize=12)
    ax2.grid(True, linestyle='--', alpha=0.7)
    
    # Marquem el màxim global (energia al lag 0)
    ax2.plot(0, autocorr[0], 'ko', markersize=6, label='Màxim Absolut (Energia, $r[0]$)')
    
    # Marquem el primer màxim secundari trobat
    ax2.plot(T0_ms, autocorr[max_sec_idx], 'ro', markersize=8, label='Primer Màxim Secundari')
    
    # Dibuixem una fletxa assenyalant el màxim secundari
    ax2.annotate('Màxim cercat\nper al pitch', 
                 xy=(T0_ms, autocorr[max_sec_idx]), 
                 xytext=(T0_ms + 2, autocorr[max_sec_idx]),
                 arrowprops=dict(facecolor='black', shrink=0.05, width=1.5, headwidth=6))
    
    # Ombres per delimitar la zona de cerca vàlida (npitch_min a npitch_max)
    zona_cerca_start = lag_ms[lag_min_idx]
    zona_cerca_end = lag_ms[lag_max_idx]
    ax2.axvspan(zona_cerca_start, zona_cerca_end, color='yellow', alpha=0.2, label='Zona de cerca F0 (50-500Hz)')

    ax2.legend(loc='upper right')

    # Guardar i mostrar
    plt.tight_layout()
    plt.savefig('grafica_autocorrelacio.png', dpi=300, bbox_inches='tight')
    print("Gràfica guardada com 'grafica_autocorrelacio.png'")
    plt.show()

if __name__ == "__main__":
    # Nom del fitxer per defecte. Es pot passar per paràmetre al terminal
    fitxer_dades = "frame_data.txt"
    if len(sys.argv) > 1:
        fitxer_dades = sys.argv[1]
        
    # Freqüència de mostreig típica de les bases de dades de veu (16 kHz)
    # Assegura't de canviar-la si els teus àudios són a 8000 Hz o similar.
    plot_autocorrelation(fitxer_dades, sample_rate=16000)