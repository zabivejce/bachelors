import numpy as np
from scipy.signal import firwin

# Nastavte podle vašeho projektu
sample_rate = 2048000  # Vzorkovací frekvence vašeho SDR (např. 2.4 MHz)
cutoff_freq = 80000   # Kde chceme řezat (100 kHz propustí jednu FM stanici, zbytek zabije)
num_taps = 63          # Počet koeficientů (musí být liché číslo, 63 je dobrý kompromis mezi rychlostí a kvalitou filtrace)

# Výpočet koeficientů (použije se implicitně Hammingovo okno)
taps = firwin(num_taps, cutoff_freq, fs=sample_rate)

# Vygenerování C++ kódu pro snadné zkopírování
print("std::vector<float> my_taps = {")
print("    " + ", ".join(f"{t:.6f}f" for t in taps))
print("};")