### Assignment 4: Go Back N and Selective Repeat Protocol

- This assignment contains implementation of basic Selective-Repeat and Go-Back-N protocol.
- Each directory contains 3 files,
  - `sender.py` : Generates and transmits packets using TCP sockets.
  - `receiver.py` : Accepts the packets and transmits the acknowledgements to the sender using TCP sockets
  - `runner.py` : Helper file to run both sender and receiver together on various params and collect the result.

### Commands to run

- `python <dirname>/sender.py` will start the sender with default TCP port 10001. It will wait to connect with receiver on port 10000 and on localhost by default.
- Similarly, `python <dirname>/receiver.py` will start the receiver with default TCP port 10000. It will wait to receive connection from sender on port 10001 and on localhost by default.
- `python <dirname>/runner.py` will run both the server & receiver on various parameters and collect the results in `results` directory.
- `<dirname>` can be either `Go_Back_N` or `Selective_Repeat`
- <b><i>`Note`</i></b>: You can pass `'--help'` or `'-h'` as param to see the list of options available in any python script.

- `python results/plotter.py` will generate the plots for all the results collected in `results` directory and save it in `plots` directory. It was used for adding plots in the report.
