# ndn6-tools

This repository contains tools for yoursunny's (now discontinued) NDN6 router.

## Build Instructions

`Makefile` assumes:

* OS is Ubuntu 16.04
* [ndn-cxx](http://named-data.net/doc/ndn-cxx/) has been installed from source code

To compile and install:

    make
    sudo make install

To uninstall:

    sudo make uninstall

## Available Tools

[facemon](facemon.md): log when a face is created or destroyed

[prefix-allocate](prefix-allocate.md): allocate a prefix to requesting face

[prefix-request](prefix-request.md): register a prefix to requesting face, where the prefix is determined by a server process that knows a shared secret

[register-prefix-cmd](register-prefix-cmd.md): prepare a prefix registration command

[serve-certs](serve-certs.md): serve certificates

[tap-tunnel](tap-tunnel.md): create an Ethernet tunnel over NDN
