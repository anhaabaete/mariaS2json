#!/bin/bash

set -x

gcc src/main.c inc/ini.c -o mariaS2json -lmysqlclient -lcrypto -lssl

cp ~/tecnologia/mariaS2json/mariaS2json /usr/lib/cgi-bin/mariaS2jsonls
