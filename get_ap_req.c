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
#define MAX_IO_TRIES 1024

void usage(char *progname)
{
    fprintf(stderr, "Usage: %s fqdn [service [output]]\n", progname);
}

int main(int argc, char **argv)
{
    int rv = 0;
    struct hostent *host;
    FILE *output = stdout;
    size_t length, pos;
    int tries = 0;
    char *cp, *progname = NULL;
    char *service = "HTTP";
    char *hostname;
    char full_hname[MAXHOSTNAMELEN];
    krb5_error_code retval;
    krb5_ccache ccdef;
    krb5_data packet, inbuf;
    krb5_context context = NULL;
    krb5_auth_context auth_context = NULL;

    setlocale(LC_ALL, "");
    progname = GET_PROGNAME(argv[0]);

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

    inbuf.data = hostname;
    inbuf.length = strlen(hostname);

    if ((retval =
	 krb5_mk_req(context, &auth_context, 0, service, full_hname,
		     &inbuf, ccdef, &packet))) {
	com_err(progname, retval, "while preparing AP_REQ");
	rv = 1;
	goto cleanup;
    }

    tries = 0;
    length = (size_t) packet.length;
    for (pos = 0; length > 0; length -= pos) {
	if (tries <= MAX_IO_TRIES)
	    tries++;
	else {
	    fprintf(stderr, "%s: IO failed while write AP_REQ msg\n", progname);
	    rv = 1;
	    goto cleanup;
	}
	pos = fwrite((packet.data + pos), sizeof(char), length, output);
        fflush(stdout);
    }

  cleanup:

    krb5_free_data_contents(context, &packet);
      
    if (auth_context) {
	krb5_auth_con_setrcache(context, auth_context, NULL);
	krb5_auth_con_free(context, auth_context);
    }

    if (context)
	krb5_free_context(context);

    if (output && output != stdout)
	fclose(output);

    exit(rv);
}
