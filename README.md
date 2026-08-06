# sucorrpilot

A C-based controlled-source seismic data cross-correlation program with Seismic Unix (SU) format support.

The program provides serial and OpenMP parallel implementations for efficient waveform cross-correlation using FFT-based methods.

The program reads seismic data in Seismic Unix (SU) format and performs waveform cross-correlation between seismic records and pilot signals using FFT-based methods.


## Implementations

Two implementations are provided:


- [sucorrpilot](sucorrpilot/) : serial version

- [sucorrpilot_omp](sucorrpilot_omp/) : OpenMP parallel version


## Dependencies

sucorrpilot uses several functions provided by Seismic Unix (SU).

The program is developed and tested with:

- Seismic Unix (SU) release 44R24
- GCC compiler
- Linux operating system


The program uses SU components including:

- SU trace data structure (`segy`)
- SU trace input/output functions
- SU FFT routines
- SU mathematical libraries


Before compilation, please install Seismic Unix and set the environment variable:

```bash
export CWPROOT=/path/to/cwp
```


The Makefile uses:


SU header files:

```text
${CWPROOT}/include
```


SU libraries:

```text
${CWPROOT}/lib
```



## Compilation


### Serial version

Compile the serial implementation:

```bash
cd sucorrpilot
make
```


The executable will be generated as:

```text
../../bin/sucorrpilot
```



### OpenMP version

Compile the OpenMP parallel implementation:

```bash
cd sucorrpilot_omp
make
```


The executable will be generated as:

```text
../../bin/sucorrpilot_omp
```


The OpenMP version requires OpenMP support:

```text
-fopenmp
```



## Usage

After successful compilation, run the executable without arguments to display the usage information.

Serial version:

```bash
sucorrpilot
```


OpenMP version:

```bash
sucorrpilot_omp
```


The program provides detailed parameter descriptions and runtime information through the built-in help message.



## Example

A simple example is provided in the [example](example/) directory.

The example demonstrates:

- seismic data input in SU format
- pilot signal preparation
- checking the output correlation result


See [example/README.md](example/README.md) for detailed instructions.



## License

This project is released under the MIT License.
