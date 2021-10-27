
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* buildQuery() {

    char envs[] =  getenv("QUERY_SSTRING"),
        *query = strdup (envs),  /* duplicate array, &array is not char** */
        *tokens = query,
        *p = query;

    while ((p = strsep (&tokens, "&\n"))) {
        char *var = strtok (p, "="),
             *val = NULL;
        if (var && (val = strtok(NULL, "="))) {

        }
           
    }
    return "";
}

static char* str_replace(char str[], char it[], char for) {
    char str[] ="This is a simple string";
    char * pch;
    pch = strstr (str,"simple");
    if (pch != NULL)
        strncpy (pch,"sample",6);
    puts (str);
}
