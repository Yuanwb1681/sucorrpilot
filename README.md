# sucorrpilot
A C-based controlled-source seismic data cross-correlation program with SU format support. Includes serial and OpenMP parallel implementations for efficient waveform correlation using FFT-based methods.

The program reads seismic data in Seismic Unix (SU) format and performs waveform cross-correlation between seismic records and pilot signals using FFT-based methods.

Two implementations are provided:

- `sucorrpilot` : serial version
- `sucorrpilot_omp` : OpenMP parallel version

## Example

A simple example is provided in the `example/` directory.

The example demonstrates:

- seismic data input in SU format
- pilot signal preparation
- checking the output correlation result

See [example/README.md](example/README.md) for detailed instructions.
