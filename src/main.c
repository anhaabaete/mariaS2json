/**
 * mariaS2json
 * This a program/service to output json from MariaDB or MySQL
 * in a HTTP protocol pattern
 **/

#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>
#include <time.h>
#include "headers/ini.h"

#include "headers/marias2json.h"


int main() {

    the_time = getTime();

    
    configuration cfg;

    // Get configuration
    char* filepath = "s2.conf";
    if (ini_parse(filepath, handler, &cfg) < 0) {
        s2Error("Service cconfiguration file problem");
    } 

    if (cfg.keyseed==NULL) {
        s2Error("Key seed dont define in conf file");
    }

    //get querystrung from url
    char *envs = (getenv("QUERY_STRING")!=NULL) ? getenv("QUERY_STRING") : "";

    //set file SQL as service name "_s"
    char* file_serv_name = getService(strdup(envs));

    
    //set connection 
     MYSQL *conn;

     conn = mysql_init(NULL);
     /* Connect to database */
     if (!mysql_real_connect(conn, cfg.server,
         cfg.user, cfg.password, cfg.database, 0, NULL, 0)) {
        s2Error(mysql_error(conn));
    }


    //Check security key handshake if is not onle for login.sql
    if (strcmp("login.sql",file_serv_name)!=0) {
        checkSecurityKey(conn);
    } else {
        login = 1;
    }

    //if go to login or checked security key, create a new key
    createSecurityKey(cfg.keyseed);

    
    //flush JSON
    flushJsonData(conn, envs, file_serv_name, cfg.debug);


    /**
     * UPDATE DA SECURITY KEY
     * If login=0 and result from query
     * or login=0 
     **/
    updateSKC(conn);
      
    mysql_close(conn);

    return 0;
}
