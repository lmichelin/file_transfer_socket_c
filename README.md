# IN202 ENSTA Network project

This project is part of the evaluation of the IN202 course at ENSTA ParisTech.

The aim of this project is to implement a file transfer program written in C by using socket communication.

To run this project use the following commands from the project directory :

```bash

make
./server

```
Put the file to transfer in the project directory.

Don't forget to run the client from a different directory to avoid file overwriting :

```bash

mkdir client_folder
cd client_folder
../client myfile

```
