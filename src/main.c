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

#include "../inc/ini.h"

//data base variable accesss
typedef struct {
    char* server;
    char* database;
    char* user;
    char* password;
    char* pathconf;
    char* debug;
    char* keyseed;
} configuration;

uint8_t login = 0;
uint8_t loged = 0;
char* security_key;
char* old_security;
char* the_time;
char user_id[20];



/**
 * httpHeader
 * output for a HTTP header
 **/
static void httpHeader(u_int8_t code) {
    printf("Status: %i\r\n", code);
    printf("Content-type:application/json\r\n");
    if (code==200) {
        printf("Set-Cookie: securityKey=%s\r\n", security_key);
    }
    printf("\r\n");
}


/**
 * s2error
 * output error in a json pattern
 **/
static void s2Error(const char* info) {
    httpHeader(250);
    printf("{\"error\":1,\"data\":\"%s\",\"SKC\":\"%s\"}",info,old_security);
     
    exit(0);
}

/**
 * md5hash
 * Simple md5 to security key
 **/
char* md5Hash(char *str) {
    unsigned char digest[16];
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, str, strlen(str));
    MD5_Final(digest, &ctx);

    char buf[sizeof digest * 2 + 1];
    for (int i = 0, j = 0; i < 16; i++, j+=2)
    {
        sprintf(buf+j, "%02x", digest[i]);
    }
    buf[sizeof digest * 2] = 0;
    
    return strdup(buf);
}

/**
 * handler
 * get data from a conf file
 **/
static int handler(void* user, const char* section, const char* name, const char* value)
{
    configuration* pcfg = (configuration*)user;
    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MATCH("basic", "server")) {
        pcfg->server = strdup(value);
    } else if (MATCH("basic", "database")) {
        pcfg->database = strdup(value);
    } else if (MATCH("basic", "user")) {
        pcfg->user = strdup(value);
    } else if (MATCH("basic", "password")) {
        pcfg->password = strdup(value);
    } else if (MATCH("basic", "pathconf")) {
        pcfg->pathconf = strdup(value);
    } else if (MATCH("basic", "debug")) {
        pcfg->debug = strdup(value);
    } else if (MATCH("basic", "keyseed")) {
        pcfg->keyseed = strdup(value);
    } else {
        
        return 0;  /* section ivalid */
    }
    return 1;
}

/**
 * replaceWord
 * Simple replace word function
 **/
static char* replaceWord(const char* s, const char* oldW, const char* newW)
{
    char* result;
    int i, cnt = 0;
    int newWlen = strlen(newW);
    int oldWlen = strlen(oldW);
  
    // Counting the number of times old word
    // occur in the string
    for (i = 0; s[i] != '\0'; i++) {
        if (strstr(&s[i], oldW) == &s[i]) {
            cnt++;
  
            // Jumping to index after the old word.
            i += oldWlen - 1;
        }
    }
  
    // Making new string of enough length
    result = (char*)malloc(i + cnt * (newWlen - oldWlen) + 1);
  
    i = 0;
    while (*s) {
        // compare the substring with the result
        if (strstr(s, oldW) == s) {
            strcpy(&result[i], newW);
            i += newWlen;
            s += oldWlen;
        }
        else
            result[i++] = *s++;
    }
  
    result[i] = '\0';
    return result;
}

/**
 * getService
 * Get the name of service in a especific variable of querystring "_s"
 * it define how file ([..].sql) will read 
 **/
static char* getService(char *url_qs) {
    if (!(url_qs = strstr(url_qs,"_s"))) {
        s2Error("request without service name");
        exit(0);
    }
    char* service = strtok(strdup(url_qs), "&");
    return strcat(service+3,".sql");
}

static void checkSecurityKey(MYSQL *conn) {
    
    char* cookie_key = getenv("HTTP_COOKIE") ? getenv("HTTP_COOKIE") : "";
    if (strlen(cookie_key)==0) {
        s2Error("Denied Access 100");
    }

    char * strcookie;
    if (!(strcookie = strstr(cookie_key,"securityKey="))) {
        s2Error("Denied Access 200");
    }

 
    char* ckey = strtok(strdup(strcookie), "&");
    ckey = ckey+12;

    old_security = ckey;

    char query_check[130] = "SELECT id FROM _user WHERE session_hash='";
    strcat(query_check,ckey);
    strcat(query_check,"' AND valid_date >= '");
    strcat(query_check,the_time);
    strcat(query_check,"';");

    if (mysql_query(conn, query_check)) {
        s2Error("Denied Access 300");
    }
    
    MYSQL_RES *res;
    MYSQL_ROW row;

    res = mysql_use_result(conn);

    if ((row = mysql_fetch_row(res)) != NULL) {
        loged = 1;
    } else {
        s2Error("Denied Access 400");
    }

    mysql_free_result(res);
}

static char* getTime() {
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [20];

    time (&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer,20,"%Y-%m-%d %H:%M:%S",timeinfo);

    return strdup(buffer);

}

static void createSecurityKey(char* getseed) {
    
    char str_to_key[30];

    strcpy(str_to_key,strdup(the_time));
 
    strcat(str_to_key,getseed);

    security_key = md5Hash(str_to_key);
}
/**
 * buildQuery
 * This function get the querystring from URL and pick a file service ([..].sql)
 * Replace data from querystring into a SQL string
 **/
static char *buildQuery(char* envs, char* file_serv_name) {

    char *query = strdup(envs),  /* duplicate array, &array is not char** */
        *tokens = query,
        *p = query;

    if ((strstr(query,"user=")==NULL ||
        strstr(query,"password=")==NULL) &&
        strcmp("login.sql",file_serv_name)==0) {
            s2Error("To login you need two arguments user and password");
    }
    
    FILE *fp;
    long lSize;
    char *buffer;
    fp = fopen(file_serv_name, "rb");
    if (!fp) {
      s2Error("Erro on open file service");
    }
    fseek( fp , 0L , SEEK_END);
    lSize = ftell(fp);
    rewind( fp );

   buffer = calloc( 1, lSize+1 );
    if( !buffer ) fclose(fp),fputs("memory alloc fails",stderr),exit(1);

    /* copy the file into the buffer */
    if( 1!=fread( buffer , lSize, 1 , fp) )
    fclose(fp),free(buffer),fputs("entire read fails",stderr),exit(1);

    /* do your work here, buffer is a string contains the whole text */

    fclose(fp);
    char* fimq; 
    fimq = strdup(buffer);

    free(buffer);

    while ((p = strsep(&tokens, "&"))) {
        
        char *var = strtok (p, "="),
             *val = NULL;
        if (var && (val = strtok(NULL, "="))) {
            char * pch;
            pch = strstr(strdup(fimq),var);
            if (pch != NULL) {
                char rvar[] = "$";
                strcat(rvar,var);
                char* repl = replaceWord(fimq, rvar, strdup(val));
                fimq = repl;
            }
        }
           
    }
    
    fimq[strlen(fimq)] = '\0';
    return fimq;
}

/**
 * type_enbrace
 * This function choose a double quotes when data type need
 **/
char* type_enbrace(enum enum_field_types type) {
    switch (type) {
        case MYSQL_TYPE_VAR_STRING || 
                MYSQL_TYPE_DATE || 
                MYSQL_TYPE_VARCHAR ||
                MYSQL_TYPE_STRING ||
                MYSQL_TYPE_VAR_STRING ||
                MYSQL_TYPE_TIME || 
                MYSQL_TYPE_TIMESTAMP ||
                MYSQL_TYPE_TIMESTAMP2 ||
                MYSQL_TYPE_DATETIME ||
                MYSQL_TYPE_DATETIME2 ||
                MYSQL_TYPE_DATE ||
                MYSQL_TYPE_ENUM ||
                MYSQL_TYPE_TIME2 ||
                MYSQL_TYPE_BLOB ||
                MYSQL_TYPE_MEDIUM_BLOB:

            return "\"";
        default:
            return "";
    }
}

static void flushJsonData(MYSQL *conn, char *envs, char * file_serv_name) {

    MYSQL_RES *res;
    MYSQL_ROW row;

    configuration cfg;

     /* send SQL query */
    const char *sqlquery = buildQuery(envs,file_serv_name);
    if (mysql_query(conn,sqlquery)) {
        s2Error(mysql_error(conn));
    }

    res = mysql_use_result(conn);


    // get info fields to build JSON
    MYSQL_FIELD *fields;
    fields = mysql_fetch_fields(res);
    unsigned int num_fields = mysql_num_fields(res);
    unsigned int i;

    httpHeader(200);

    /********************************
     * Start flushing data response *
     *******************************/
    
    
    char* comma = ""; // comma start void becouse only second row need separate with it
    printf("{\"error\":0,\"data\":["); // erro 0 (nothing), and data populate open

    // looping rows data
    while ((row = mysql_fetch_row(res)) != NULL) 
    {
        printf("%s{",comma); //put a comma whether first row it's void or second row it's filled
        char* commaf = ""; // set comma fields, it's start void

        for(i = 0; i < num_fields; i++) {

            char* b = type_enbrace(fields[i].type);

            printf("%s\"%s\":\"%s\"", 
                commaf, //put a comma wheter first field it's will be void, and second it's will be filed
                fields[i].name, 
                row[i]);

            commaf = ","; // filled comma to between the fields

            if (login==1 && strcmp(fields[i].name,"id")==0) {
                loged=1;
                sprintf(user_id,"%s",row[i]);
            }
        }

        printf("}");
        comma = ","; // fill comma to the second row
    }   
    printf("]");
    
    if (strcmp(cfg.debug,"true")==0) {
        printf(",\"execQuery\":\"%s\"",sqlquery);
    }
    
    printf("}"); // close JSON


    /* close connection */
    mysql_free_result(res);
}


static void updateSKC(MYSQL *conn) {
    if (loged==1) {
        char query_up_u[160] = "UPDATE _user SET valid_date=DATE_ADD(NOW(), INTERVAL 6 HOUR), session_hash='";
        
        strcat(query_up_u,security_key);

        strcat(query_up_u,"' WHERE id =");
        strcat(query_up_u,user_id);
            
        if (mysql_query(conn,query_up_u)) {
            s2Error(mysql_error(conn));
        }
    }
}

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
     MYSQL_RES *res;
     MYSQL_ROW row;

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
    flushJsonData(conn, envs, file_serv_name);


    /**
     * UPDATE DA SECURITY KEY
     * If login=0 and result from query
     * or login=0 
     **/
    updateSKC(conn);
      
    mysql_close(conn);

    return 0;
}