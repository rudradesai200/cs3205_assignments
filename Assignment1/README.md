## Instructions to compile

- `make` will set up all the folders and create executables
- `make clean` will remove all the created executables and files.

## How to Run?

- `./emailserver <port_no>` - starts the emailserver
- `./emailclient <serverip> <port_no>` - starts the emailclient
- For ex,
  - `./emailserver 12001`
  - `./emailclient 127.0.0.1 12001`

## Other instructions

- Emailserver should always be up and running before emailclient runs.
- Mail info will stay persistent across user sessions as well as server sessions ,provided that emailserver is not closed while a emailclient is running.
- All the attached screenshots are run on remote host, which will be reproduced while viva presentation.
