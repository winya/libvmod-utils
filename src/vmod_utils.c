#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <ctype.h>

#include "vas.h"
#include "vrt.h"
#include "vcc_if.h"
#include "bin/varnishd/cache.h"

struct vmod_utils_data {
	char* hostname;
};

void* utils_init()
{
	struct vmod_utils_data* data = malloc(sizeof(struct vmod_utils_data));
	AN(data);

	data->hostname = malloc(HOST_NAME_MAX + 1);
	AN(data->hostname);

	if(gethostname(data->hostname, HOST_NAME_MAX) == -1) {
		syslog(LOG_ERR, "gethostname failed: %s", strerror(errno));
		strcpy(data->hostname, "<unknown>");
	}
	return (void*)data;
}

void utils_free(void* priv)
{
	struct vmod_utils_data* data = (struct vmod_utils_data*)priv;

#define freez(x) do { if (x) free(x); x = NULL; } while (0);
	freez(data->hostname);
	freez(data);
#undef freez
}

int
init_function(struct vmod_priv *priv, const struct VCL_conf *conf)
{
	if(priv->priv == NULL) {
		priv->priv = utils_init();
		priv->free = utils_free;
	}
	return (0);
}

double __match_proto__()
vmod_real(struct sess *sp, const char *p, double d)
{
	char *e;
	double r;

	(void)sp;
	if (p == NULL)
		return (d);

	e = NULL;
	r = strtod(p, &e);

	if (e == NULL)
		return (d);
	if (*e != '\0')
		return (d);
	return (r);
}

const char*
vmod_hostname(struct sess * sp, struct vmod_priv *priv)
{
	struct vmod_utils_data* data = (struct vmod_utils_data*)priv->priv;
	(void)sp;
	return data->hostname;
}

const char*
vmod_timestamp(struct sess * sp)
{
	char *p;

	(void)sp;
	#define TIMESTAMP_LENGTH 64
	AN(p = WS_Alloc(sp->http->ws, TIMESTAMP_LENGTH));
	snprintf(p, TIMESTAMP_LENGTH, "%.9f", TIM_real());
	return p;
}

unsigned int
vmod_exists(struct sess *sp, const char *path)
{
	struct stat st;

	(void)sp;
	return (stat(path, &st) == 0);
}

const char*
vmod_split_random(const char* str, const char* delim) {
    char *s = strdup(str);

    size_t tokens_alloc = 1;
    size_t tokens_used = 0;
    char **tokens = calloc(tokens_alloc, sizeof(char*));
    char *token;
    int i;

    char *token, *strtok_ctx;
    for (token = strtok_r(s, delim, &strtok_ctx);
            token != NULL;
            token = strtok_r(NULL, delim, &strtok_ctx)) {
        // check if we need to allocate more space for tokens
        if (tokens_used == tokens_alloc) {
            tokens_alloc *= 2;
            tokens = realloc(tokens, tokens_alloc * sizeof(char*));
        }
        tokens[tokens_used++] = strdup(token);
    }

    // cleanup
    if (tokens_used == 0) {
        free(tokens);
        tokens = NULL;
        // return original string
        return s;
    } else {
    	// select a random string from the list
    	i = rand() % tokens_used;
        token = strdup(tokens[i]);
    }
    
    free(tokens);
    free(s);

    return token;
}
