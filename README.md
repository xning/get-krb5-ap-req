## About get\_ap\_req

### What does this program do?

This program will output *KRB\_AP\_REQ* message containing client credentials, as
produced by [krb5 _ mk _ req()](http://web.mit.edu/kerberos/krb5-latest/doc/appdev/refs/api/krb5_mk_req.html), sometime this is useful.
While the bad news is that now we have no shell tools to process cts mode encryptions.

### How to use it?

1. Install packages

 - On RHEL-like systems
 
    `yum -y install krb5-workstation libcom_err`

 - On Ubuntu

    `sudo apt-get -y install krb5-user libcomerr2`  

- First, get the initial credentials.

    `kinit`  

- Usage

    `get_ap_req fqdn [service [output]]`

- In *get\_ap\_req* arguments, service default is 'HTTP', and output is 'stdout'. Notice, if successful, *get\_ap\_req* will get binary data.
  So you terminal will full of chaotic characters and colors. Pls try `reset`.

- If *get\_ap\_req* is not in /*bin directories, pls use `./get_ap_req ...` instead of `get_ap_req ...`.

- F.g.

     `get_ap_req brew.example.com`  
     `get_ap_req brew.example.com HTTP`  
     `get_ap_req brew.example.com HTTP /tmp/msg`

- To get the help info, pls try

    `get_ap_req -h`  
    `get_ap_req --help`  

### How to build this program

- Pls install the follwing libs

 - On RHEL-like systems

    `yum -y install krb5-devel libcom_err-devel`

 - On Ubuntu

     `sudo apt-get -y install libkrb5-dev comerr-dev`

- Clone

     `git clone https://github.com/xning/get-krb5-ap-req.git`
  
- Build

 - Enter the source dir
 
      `cd get-krb5-ap-req`

 - Enable debug if need

    `gcc -ggdb -o get_ap_req.debug get_ap_req.c $(pkg-config --libs --cflags com_err) -I/usr/include/krb5/ -lkrb5`
    
 - As regular program
 
    `gcc -O2 -o get_ap_req get_ap_req.c $(pkg-config --libs --cflags com_err) -I/usr/include/krb5/ -lkrb5`
    
 - Install

    `install get_ap_req.debug DIR && chmod ugo+x DIR/get_ap_req.debug`

    Or

    `install get_ap_req DIR && chmod ugo+x DIR/get_ap_req.debug`

### Why not use the [GSSAPI](http://en.wikipedia.org/wiki/GSSAPI)?

Now I'm not sure that GSSAPI is enough or not.
