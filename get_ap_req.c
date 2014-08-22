#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <com_err.h>
#include <krb5.h>

#include <netdb.h>
extern int h_errno;

#define GET_PROGNAME(x) (strrchr((x), '/') ? strrchr((x), '/')+1 : (x))
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif
#define MAX_IO_TRIES 64

#define VERSION "0.1.2"
#define AUTHOR "anzhou94"

void usage(char *progname)
{
    fprintf(stderr, "Usage: %s fqdn [service [output]]\n", progname);
    fprintf(stderr, "Version: %s\n", VERSION);
}

int main(int argc, char **argv)
{
    int rv = 0;
    struct hostent *host;
    FILE *output = stdout;
    size_t length, pos;
    char *pos_ptr = NULL;
    int tries = 0;
    char *cp, *progname = NULL;
    char *service = "HTTP";
    char *hostname;
    char full_hname[MAXHOSTNAMELEN];
    krb5_error_code retval;
    krb5_ccache ccdef;
    krb5_creds in_creds, *creds;
    krb5_principal client, server;
    char *server_principal_str = NULL;
    char *realm = NULL;
    krb5_data packet;
    krb5_context context = NULL;
    krb5_auth_context auth_context = NULL;

    setlocale(LC_ALL, "");
    progname = GET_PROGNAME(argv[0]);

    memset(&in_creds, 0, sizeof(in_creds));
    memset(&server, 0, sizeof(server));
    memset(&client, 0, sizeof(client));

    if (argc != 2 && argc != 3 && argc != 4) {
	usage(progname);
	rv = 1;
	goto cleanup;
    }

    if (argv[1][0] == '-') {
	usage(progname);
	rv = 1;
	goto cleanup;
    }

    if (argc >= 2)
	hostname = argv[1];
    if (argc >= 3)
	service = argv[2];
    if (argc == 4) {
	output = fopen(argv[3], "w");
	if (!output) {
	    fprintf(stderr, "%s: cannot open\n", argv[3]);
	    rv = 1;
	    goto cleanup;
	}
    }

    /* Why shall we suppose that we always get a hostname instead of an IP address? */
    if ((host = gethostbyname(hostname)) == NULL) {
	fprintf(stderr, "%s: unknown host\n", hostname);
	rv = 1;
	goto cleanup;
    }

    strncpy(full_hname, host->h_name, sizeof(full_hname) - 1);
    full_hname[sizeof(full_hname) - 1] = '\0';

    for (cp = full_hname; *cp; cp++)
	if (isupper((int) *cp))
	    *cp = tolower((int) *cp);

    if (!isatty(fileno(stdin)))
	setvbuf(stdin, 0, _IONBF, 0);
    if (!isatty(fileno(stdout)))
	setvbuf(stdout, 0, _IONBF, 0);
    if (!isatty(fileno(stderr)))
	setvbuf(stderr, 0, _IONBF, 0);

    retval = krb5_init_context(&context);
    if (retval) {
	com_err(progname, retval, "while initializing krb5");
	rv = 1;
	goto cleanup;
    }

    if ((retval = krb5_cc_default(context, &ccdef))) {
	com_err(progname, retval, "while getting default ccache");
	rv = 1;
	goto cleanup;
    }

    retval = krb5_get_default_realm(context, &realm);
    if (retval) {
	com_err(progname, retval, "while getting the default realm");
	rv = 1;
	goto cleanup;
    }

    server_principal_str =
	(char *) malloc(strlen(service) + strlen("/") + strlen(hostname) +
			strlen("@") + strlen(realm) + 1);

    if (!server_principal_str) {
	fprintf(stderr, "Failed to alloc mem\n");
	rv = 1;
	goto cleanup;
    }


    memset(server_principal_str, '\0',
	   strlen(service) + strlen("/") + strlen(hostname) + strlen("@") +
	   strlen(realm) + 1);
    memcpy(server_principal_str + strlen(server_principal_str),
	   service, strlen(service));
    memcpy(server_principal_str + strlen(server_principal_str), "/",
	   strlen("/"));
    memcpy(server_principal_str + strlen(server_principal_str), hostname,
	   strlen(hostname));
    memcpy(server_principal_str + strlen(server_principal_str), "@",
	   strlen("@"));
    memcpy(server_principal_str + strlen(server_principal_str), realm,
	   strlen(realm));
    server_principal_str[strlen(server_principal_str)] = '\0';

    if ((retval = krb5_parse_name(context, server_principal_str, &server))) {
	com_err(progname, retval,
		"while parsing the server principle string");
	rv = 1;
	goto cleanup;
    }

    if ((retval = krb5_cc_get_principal(context, ccdef, &client))) {
	com_err(progname, retval, "while getting the client principal");
	rv = 1;
	goto cleanup;
    }

    in_creds.client = client;
    in_creds.server = server;

    if ((retval = krb5_auth_con_init(context, &auth_context))) {
	com_err(progname, retval, "while init auth_context");
	rv = 1;
	goto cleanup;
    }

    if ((retval =
	 krb5_auth_con_setflags(context, auth_context,
				KRB5_AUTH_CONTEXT_DO_SEQUENCE |
				KRB5_AUTH_CONTEXT_DO_TIME))) {
	com_err(progname, retval, "while setting auth_context flags");
	rv = 1;
	goto cleanup;
    }

    if ((retval =
	 krb5_get_credentials(context, 0, ccdef, &in_creds, &creds))) {
	com_err(progname, retval, "while getting the credential");
	rv = 1;
	goto cleanup;
    }

    if ((retval =
	 krb5_mk_req_extended(context, &auth_context,
			      AP_OPTS_MUTUAL_REQUIRED, NULL, creds,
			      &packet))) {
	com_err(progname, retval, "while preparing AP_REQ");
	rv = 1;
	goto cleanup;
    }

    tries = 0;
    length = (size_t) packet.length;
    pos_ptr = (char *) packet.data;
    for (pos = 0; length > 0; length -= pos) {
	pos_ptr += pos;
	if (tries < MAX_IO_TRIES)
	    tries++;
	else {
	    fprintf(stderr, "%s: IO failed while write AP_REQ msg\n",
		    progname);
	    rv = 1;
	    goto cleanup;
	}
	pos = fwrite(pos_ptr, sizeof(char), length, output);
	fflush(output);
    }

    krb5_free_principal(context, server);
    krb5_free_principal(context, client);

    krb5_free_data_contents(context, &packet);

  cleanup:

    if (server_principal_str) {
	free(server_principal_str);
    }

    if (realm) {
	krb5_free_default_realm(context, realm);
    }

    if (auth_context) {
	krb5_auth_con_setrcache(context, auth_context, NULL);
	krb5_auth_con_free(context, auth_context);
    }

    if (context)
	krb5_free_context(context);

    if (output && output != stdout && output != stderr && output != stdin)
	fclose(output);

    exit(rv);
}
