# pseudo-database-cpp-tcp

Updated pseudo people database version which was realised like a client-server programm. Here is a client which send to server requests (add person, edit person by PESEL[^1], remove person by PESEL, get person by PESEL) and server which execute client's requests via TCP/IP protocol.

## Building a programm

### Build a server

For building a server programm please type in a terminal

```
make main_server
```

### Run a server

For running a server please type in a terminal

```
main_server
```

### Build a client

For building a client please type in a terminal

```
make main_client
```

### Run a client

For running a client please type in a terminal

```
main_client <server ip>
```

> Note: the ip of server is ip of your device in Internet.

## Work with programms

### About server

If there are not any events from clients to server for 60 seconds, the server will turn off. You can turn off a server using `CTRL + C`

### About client

Press `CTRL + C` or type `quit` to turn off the programm

- type `add <name> <surname> <pesel>` to add to database a record
- type `edit <selected pesel> <name> <surname> <pesel>` to edit a record by PESEL
- type `remove <selected pesel>` to remove a record from a database
- type `get <selected pesel>` to get a record from a database
- press `CTRL + C` or type `quit` to turn off the programm

[^1]: PESEL is the national identification number used in Poland.
