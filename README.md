[![Build Status](https://api.travis-ci.org/JehanneOS/jehanne.svg?branch=master)](https://travis-ci.org/JehanneOS/jehanne)
[![Coverity Badge](https://scan.coverity.com/projects/7364/badge.svg)](https://scan.coverity.com/projects/jehanne)

# Jehanne

Jehanne es un sistema operativo [simple][simplicity].

Jehanne tiene ancestros de alta alcurnia:

- La mayor parte de las herramientas en espacio de usuario, mucho de su sapiencia y algunos modulos del kernel, todo ello viene desde [9front][9front]
- El kernel es un fork de [Plan9-9k][plan9-9k], el cual fue programado por Charles Forsyth
- La mayor parte del sistema de compilacion y mucho codigo valioso proviene de [Harvey OS][harvey]

Si bien el proyecto recibe su nombre de una humilde campesina, la famosa hereje [Joan of Arc][arc], porque es muy divergente de las convenciones de sus predecesores.

## Overview

Este es el repositorio principal, es de utilidad para compilar la totalidad del sistema y podrá ver el arbol de directorios a continuación:

- [arch](./arch/) Contiene un directorio para cada arquitectura, con sus respectivas cabeceras en el lenguaje de programacion C, con bibliotecas y ejecutables. Nosotros entendemos por arquitectura a cualquier tipo de maquina, tanto fisica como virtual en la que ud. pueda ejecutar codigo, por lo tanto rc es un tipo de Arq.
- [sys](./sys) Es el directorio de sistema
    * [include](./sys/include) Contiene cabeceras porables programadas en el lenguaje de programacion C 
    * [lib](./sys/lib) Contiene datos y archivos de script usados por el sistema en ejecucion
    * [man](./sys/man) Contiene paginas del manual
    * [src](./sys/src) Contiene el codigo fuente del sistema
- [doc](./doc/) Contiene documentacion de utilidad para el desarrollo del Sistema Operativo Jehanne
  
    * [license](./doc/license/) Contiene informacion detallada sobre las [licenses][lic] del Sistema Operativo Jehanne
    * [hacking](./doc/hacking/) Contiene detalles sobre la compilacion y la modificacion del Sistema Operativo Jehanne
- [hacking](./hacking) contains the utilities used to
  develop Jehanne
- [qa](./qa) contains the regression tests
- [mnt](./mnt) contains default mount targets
- [usr](./usr) contains the users' folders
- [pkgs](./pkgs) will contains the installed packages

The running system also includes supplemental folders like `/lib`,
`/cmd` and `/dev` that are bound during the boot as required.

## Build

To build Jehanne and play with it, you need to have git, golang, qemu,
gcc, binutils and bison installed.
For example on Debian GNU/Linux you should be able to get going with

	sudo aptitude install git golang build-essential flex bison qemu-system

After the repository clone, you can give a look with

	git submodule init                               # we have a lot of submodules
	git submodule update --init --recursive --remote
	./hacking/devshell.sh                            # start a shell with appropriate environment
	./hacking/continuous-build.sh                    # to build everything (will take a while)
	./hacking/runOver9P.sh                           # to start the system in QEMU
	./hacking/drawterm.sh                            # to connect Jehanne with drawterm

## Hacking

Jehanne is a work in progress.
Forks and pull requests are welcome.

In [doc/hacking](./doc/hacking/) you will find all you
need to know about its principles, design and weirdness.

There's a lot of work to do, in every area of the system.

To coordinate our efforts, we use the github issues.
To discuss (and even debate) about the design and development of Jehanne
we use the [JehanneOS mailing list][mailinglist]: please join and present
yourself and your attitudes.

[simplicity]: http://plato.stanford.edu/entries/simplicity/ "What is simplicity?"
[harvey]: http://harvey-os.org "Harvey OS"
[9front]: http://9front.org/ "THE PLAN FELL OFF"
[plan9-9k]: https://bitbucket.org/forsyth/plan9-9k "Experimental 64-bit Plan 9 kernel"
[nix]: https://github.com/rminnich/nix-os
[arc]: https://en.wikipedia.org/wiki/Joan_of_Arc "Jeanne d'Arc"
[lic]: ./LICENSE.md "A summary of Jehanne licensing"
[mailinglist]: https://groups.google.com/forum/#!forum/jehanneos

