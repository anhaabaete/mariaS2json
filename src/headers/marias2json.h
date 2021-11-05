#ifndef MARIAS2JSON_H
#define MARIAS2JSON_H

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

extern uint8_t login;
extern uint8_t loged;
extern char* security_key;
extern char* old_security;
extern char* the_time;
extern char user_id[20];

void httpHeader(u_int8_t code);

void s2Error(const char* info);

char* md5Hash(char *str);

int handler(void* user, const char* section, const char* name, const char* value);

char* replaceWord(const char* s, const char* oldW, const char* newW);

char* getService(char *url_qs);

void checkSecurityKey(MYSQL *conn);

char* getTime();

void createSecurityKey(char* getseed);

char *getQuerySQL(char* envs, char* file_serv_name);

char* type_enbrace(enum enum_field_types type);

void flushJsonData(MYSQL *conn, char *envs, char * file_serv_name, char* debug);

void updateSKC(MYSQL *conn);

#endif



