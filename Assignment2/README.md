## Instructions to compile

- `make` will set up all the folders and create executables
- `make clean` will remove all the created executables and files.

## How to Run?

### Main executable

- `./simulation` - starts the simulation.
- Note, various parameters can be passed for changing constants.
- For ex,
  - `./simulation -i 2`
  - `./simulation -s 0.001`
- Also, `./simulation help` provides the help related to the usage as well as for the other params.

### Python Runner for generating plots

- `python runner.py` will use the above executable and produce plots for various configurations of the params
- All the plots can be found in plots/ folder in the same directory as the scripts after completion.
