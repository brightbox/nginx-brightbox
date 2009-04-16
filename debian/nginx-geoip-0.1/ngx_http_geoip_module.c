/*
 * Copyright (C) SPIL GAMES
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <GeoIP.h>

/* Directive handlers */
static char *ngx_http_geoip_country_file(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

/* Variable handlers */
static ngx_int_t ngx_http_geoip_country_code(ngx_http_request_t *r,  ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_geoip_country_code3(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_geoip_country_name(ngx_http_request_t *r,  ngx_http_variable_value_t *v, uintptr_t data);

/* Directives */
static ngx_command_t  ngx_http_geoip_commands[] = {

    { ngx_string("geoip_country_file"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_http_geoip_country_file,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

	ngx_null_command
};

  
static ngx_http_module_t  ngx_http_geoip_module_ctx = {
    NULL,                                  /* preconfiguration */
    NULL,                                  /* postconfiguration */
    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */
    NULL,                                  /* create/usr/local/share/GeoIP/GeoIP.dat server configuration */
    NULL,                                  /* merge server configuration */
    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};


ngx_module_t  ngx_http_geoip_module = {
    NGX_MODULE_V1,
    &ngx_http_geoip_module_ctx,            /* module context */
    ngx_http_geoip_commands,               /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_geoip_country_code(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
	struct sockaddr_in         *sin = (struct sockaddr_in *) r->connection->sockaddr;
	ngx_http_variable_value_t *vv = v;

	vv->data = (u_char *)GeoIP_country_code_by_ipnum( (GeoIP *)data, (u_long)ntohl(sin->sin_addr.s_addr) );
	vv->len = ngx_strlen( vv->data );
	vv->valid = 1;
	vv->no_cacheable = 0;
	vv->not_found = 0;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http geoip_country_code: %V %v", &r->connection->addr_text, v);

    return NGX_OK;
}

static ngx_int_t
ngx_http_geoip_country_code3(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
	struct sockaddr_in         *sin = (struct sockaddr_in *) r->connection->sockaddr;
	ngx_http_variable_value_t *vv = v;

	vv->data = (u_char *)GeoIP_country_code3_by_ipnum( (GeoIP *)data, (u_long)ntohl(sin->sin_addr.s_addr) );
	vv->len = ngx_strlen( vv->data );
	vv->valid = 1;
	vv->no_cacheable = 0;
	vv->not_found = 0;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http geoip_country_code3: %V %v", &r->connection->addr_text, v);

    return NGX_OK;
}

static ngx_int_t
ngx_http_geoip_country_name(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
	struct sockaddr_in         *sin = (struct sockaddr_in *) r->connection->sockaddr;
	ngx_http_variable_value_t *vv = v;

	vv->data = (u_char *)GeoIP_country_name_by_ipnum( (GeoIP *)data, (u_long)ntohl(sin->sin_addr.s_addr) );
	vv->len = ngx_strlen( vv->data );
	vv->valid = 1;
	vv->no_cacheable = 0;
	vv->not_found = 0;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http geoip_country_name: %V %v", &r->connection->addr_text, v);

    return NGX_OK;
}



static char *
ngx_http_geoip_country_file(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_str_t                *value,  country_filename;
	
	ngx_http_variable_t      *country_code_var;
	ngx_str_t ngx_country_code_var_name = ngx_string("geoip_country_code");

	ngx_http_variable_t      *country_code3_var;
	ngx_str_t ngx_country_code3_var_name = ngx_string("geoip_country_code3");
	
	ngx_http_variable_t      *country_name_var;
	ngx_str_t ngx_country_name_var_name = ngx_string("geoip_country_name");

	GeoIP *gi;
	int openflags = ( GEOIP_MEMORY_CACHE | GEOIP_CHECK_CACHE );   

	value = cf->args->elts;

	country_filename = value[1];	/* Argument = GeoIP data filename */

    /* init geoip api */
	gi = GeoIP_open( (char *)country_filename.data, openflags );
	if ( !gi )
		return NGX_CONF_ERROR;

	country_code_var = ngx_http_add_variable(cf, &ngx_country_code_var_name, NGX_HTTP_VAR_CHANGEABLE );
	if (country_code_var == NULL) {
		return NGX_CONF_ERROR;
	}

	country_code3_var = ngx_http_add_variable(cf, &ngx_country_code3_var_name, NGX_HTTP_VAR_CHANGEABLE );
	if (country_code3_var == NULL) {
		return NGX_CONF_ERROR;
	}

	country_name_var = ngx_http_add_variable(cf, &ngx_country_name_var_name, NGX_HTTP_VAR_CHANGEABLE );
	if (country_name_var == NULL) {
		return NGX_CONF_ERROR;
	}

	country_code_var->get_handler = ngx_http_geoip_country_code;	/* Set country code  variable handler */
	country_code_var->data = (uintptr_t)gi;

	country_code3_var->get_handler = ngx_http_geoip_country_code3;	/* Set country code3 variable handler */
	country_code3_var->data = (uintptr_t)gi;

	country_name_var->get_handler = ngx_http_geoip_country_name;	/* Set country name  variable handler */
	country_name_var->data = (uintptr_t)gi;

	return NGX_CONF_OK;
}
