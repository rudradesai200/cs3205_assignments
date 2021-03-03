## Instructions to compile

- `make` will set up all the folders and create executables

## How to Run?

- `./emailserver <port_no>` - starts the emailserver
- `./emailclient <serverip> <port_no>` - starts the emailclient
- For ex,
  - `./emailserver 12001`
  - `./emailclient 127.0.0.1 12001`

## Other instructions

- Emailserver should always be running before emailclient runs.
- Mail info will stay persistent across user sessions as well as server sessions ,provided that emailserver is not closed while a emailclient is running.
