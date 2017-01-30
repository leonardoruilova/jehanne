[![Build Status](https://api.travis-ci.org/JehanneOS/jehanne.svg?branch=master)](https://travis-ci.org/JehanneOS/jehanne)
[![Coverity Badge](https://scan.coverity.com/projects/7364/badge.svg)](https://scan.coverity.com/projects/jehanne)

# Jehanne

Jehanne es un Sistema Operativo [simple][simplicity].

El Sistema Operativo Jehanne tiene ancestros de alta alcurnia:

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
- [hacking](./hacking) Contiene las herramientas para el desarrollo del Sistema Operativo Jehanne
- [qa](./qa) Contiene los tests de regresiones
- [mnt](./mnt) Contiene los puntos de montaje 
- [usr](./usr) Contiene los directorios de cada usuario
- [pkgs](./pkgs) Contendra los paquetes instalados

El sistema en ejecución incluye directoris suplementarios como `/lib`,
`/cmd` y `/dev` that are bound during the boot as required.

## Compilacion

Para compilar el codigo del Sistema Operativo Jehanne y jugar con el, sera necesario instalar GIT, el lenguaje de programacion GO y el emulador Qemu, el compilador GCC, Binsutils y Bison.
Por ejemplo, si ud. utiliza Debian GNU/Linux será necesario que use la siguiente sintaxis de shelll scripting

	sudo aptitude install git golang build-essential flex bison qemu-system

Despues de clonar el repositorio GIT, ud. puede continuar con la siguiente sintaxis

	git submodule init                               # we have a lot of submodules
	git submodule update --init --recursive --remote
	./hacking/devshell.sh                            # start a shell with appropriate environment
	./hacking/continuous-build.sh                    # to build everything (will take a while)
	./hacking/runOver9P.sh                           # to start the system in QEMU
	./hacking/drawterm.sh                            # to connect Jehanne with drawterm

## Hacking

El sistema Operativo Jehanne es una obra en construccion.
Los Forks y pull requests son bienvenidos.

En el directorio [doc/hacking](./doc/hacking/) ud. encontrara toda la informacion necesaria sobre los principios, el diseno y la excentricidad en nuestro proyecto.

Existe mucho trabajo por delante, en cada area de nuestro Sistema Operativo.

Para coordinar nuestros esfuerzos, usaremos los issues de github.
Para coordinar nuestro trabajo e incluso debatir sobre el diseno y el desarrollo del Sistema Operativo, nosotros utilizaremos la  [JehanneOS mailing list][mailinglist]: por favor inscribase y cuentenos quien es ud. y como puede colaborar con nosotros.

[simplicity]: http://plato.stanford.edu/entries/simplicity/ "Que es la simplicidad?"
[harvey]: http://harvey-os.org "Harvey OS"
[9front]: http://9front.org/ "El plan ha fracasado"
[plan9-9k]: https://bitbucket.org/forsyth/plan9-9k "Kernel Plan 9 experimental de 64-bit"
[nix]: https://github.com/rminnich/nix-os
[arc]: https://en.wikipedia.org/wiki/Joan_of_Arc "Jeanne d'Arc"
[lic]: ./LICENSE.md "Un sumario acerca de la licencia del Sistema Operativo Jehanne"
[mailinglist]: https://groups.google.com/forum/#!forum/jehanneos

## Licencia de la traduccion
Esta traduccion ha sido liberada a los comunes bajo licencia CC-BY-NC, su autor es Virgilio Leonardo Ruilova, su homepage es http://leonardoruilova.wordpress.com

