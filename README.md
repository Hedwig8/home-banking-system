# Home Banking System

This repo contains a program server and user that together emulate a banking system

## Instalation and Usage

 1. Download/clone the content of this repo
 2. Run the command 'make' in the directory
 3. In one terminal, run 'server \<no of threads\> \<admin password\>' to start the server
    * being each thread a bank office that will process each one a request
 4. In another terminal, run 'user \<acc no\> \<acc password\> \<op delay\> \<op\> \<op arguments\>' as a request
    * acc no: number of the account, being admin the account number 0
    * acc password : password of the account with the number in the previous argument
    * op delay: operation delay, in which the bank waits after having access to the accounts (evaluation purposes)
    * op:
      - 0: create new account, only accepted by admin, with arguments '\<new acc no\> \<initial balance\> \<password\>'
      - 1: check balance, asked by any account existing, except admin, takes no arguments
      - 2: transfer from acc1 to acc2 with arguments '\<acc2\> \<amount\>'
      - 3: server shutdown, only asked by admin, takes no arguments

## Motivation

This project was developed during the classes of Operating Systems, and the goal was to learn about multithreading, syncronization and communication between processes through FIFOs
