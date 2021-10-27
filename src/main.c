#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../inc/ini.h"


typedef struct {
    char* server;
    char* database;
    char* user;
    char* password;
    char* pathconf;
} configuration;

static void httpheader() {
    printf("Content-type:application/json\r\n\r\n");
}

static void s2Error(const char * info) {
    printf("{\"error\":1,\"data\":[\"");
    printf("%s",info);
    printf("\"]}");
}

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
    } else {
        return 0;  /* section ivalid */
    }
    return 1;
}

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

static char* getService(char *url_qs) {

    if (!(url_qs = strstr(url_qs,"_s"))) {
        s2Error("request without service name");
        exit(0);
    }

    char* service = strtok(strdup(url_qs), "&");

    return strcat(service+3,".sql");

}

static char* buildQuery() {

    char *envs = (getenv("QUERY_STRING")!=NULL) ? getenv("QUERY_STRING") : "",
        *query = strdup(envs),  /* duplicate array, &array is not char** */
        *tokens = query,
        *p = query;
    
    char ch;

    char* file_sev_name = getService(tokens);


    FILE *file;
    file = fopen(file_sev_name, "r");

    if (file == NULL) {
      s2Error("Erro on open file service");
      exit(0); 
    }
  
    char query_sql[500];
    
    int i = 0;
    while ((ch = fgetc(file)) != EOF) {
        query_sql[i] = ch;
        i++;
    } 
    fclose(file);
    
     char* fimq; 
    fimq = strdup(query_sql);
    

    while ((p = strsep (&tokens, "&"))) {
        char *var = strtok (p, "="),
             *val = NULL;
        if (var && (val = strtok(NULL, "="))) {
            char * pch;
            pch = strstr(query_sql,var);
            if (pch != NULL) {
                char rvar[] = "$";
                strcat(rvar,var);
                char* repl = replaceWord(fimq, rvar, strdup(val));
                fimq = repl;
            }
        }
           
    }
    
    return fimq;
}

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

int main() {
httpheader();

    configuration cfg;

    // Get configuration on the same path 
    char* filepath = "s2.conf";
    if (ini_parse(filepath, handler, &cfg) < 0) {
        s2Error("Service cconfiguration file problem");
      perror(filepath);

        return 1;
    } 

     MYSQL *conn;
     MYSQL_RES *res;
     MYSQL_ROW row;

     conn = mysql_init(NULL);
     /* Connect to database */
     if (!mysql_real_connect(conn, cfg.server,
         cfg.user, cfg.password, cfg.database, 0, NULL, 0)) {
        s2Error(mysql_error(conn));
        exit(1);
    }

    /* send SQL query */
    if (mysql_query(conn, buildQuery())) {
        s2Error(mysql_error(conn));
        exit(1);
    }
    res = mysql_use_result(conn);

    MYSQL_FIELD *fields;
    fields = mysql_fetch_fields(res);
    unsigned int num_fields = mysql_num_fields(res);
    unsigned int i;

    char* comma = "";
    printf("{\"error\":0,\"data\":[");
    while ((row = mysql_fetch_row(res)) != NULL)
    {
        printf("%s{",comma);
        char* commaf = "";

        for(i = 0; i < num_fields; i++) {

            char* b = type_enbrace(fields[i].type);

            printf("%s\"%s\":\"%s\"", 
                commaf,fields[i].name, row[i]);

            commaf = ",";
        }

        printf("}");
        comma = ",";
    }   
    printf("]}");


    /* close connection */
    mysql_free_result(res);
    mysql_close(conn);

    return 0;
}