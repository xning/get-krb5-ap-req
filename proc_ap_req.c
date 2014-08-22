#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
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

void usage(const char *progname)
{
    fprintf(stderr,
	    "Usage: %s path_to_req_msg[ path_to_keytab[ service]]\n",
	    progname);
}

int main(int argc, char **argv)
{
    int rv = 0;

    FILE *fd = NULL;
    size_t length = 0, pos = 0;
    char *pos_ptr = NULL;
    int tries = 0;

    char *req_msg = NULL, *keytab_file = NULL;
    krb5_keytab keytab = NULL;
    char *service = "HTTP";

    krb5_error_code retval;
    krb5_data packet, message;

    krb5_context context = NULL;
    krb5_auth_context auth_context = NULL;

    char *buf = NULL;
    struct stat stat_buf;

    krb5_principal sprinc;
    krb5_ticket *ticket = NULL;

    setlocale(LC_ALL, "");
    const char *progname = GET_PROGNAME(argv[0]);

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
	req_msg = argv[1];
    if (argc >= 3)
	keytab_file = argv[2];
    if (argc == 4)
	service = argv[3];

    retval = krb5_init_context(&context);
    if (retval) {
	com_err(argv[0], retval, "while initializing krb5");
	exit(1);
    }

    if ((retval = krb5_sname_to_principal(context, NULL, service,
					  KRB5_NT_SRV_HST, &sprinc))) {
	com_err(progname, retval, "while generating service name %s",
		service);
	rv = 1;
	goto cleanup;
    }


    retval = krb5_kt_resolve(context, keytab_file, &keytab);
    if (retval != 0) {
	com_err(progname, retval, "resolving keytab %s", keytab_file);
	goto cleanup;
    }

    if (stat(req_msg, &stat_buf) != 0) {
	fprintf(stderr, "%s: failed to get size of req_msg file %s\n",
		progname, req_msg);
	rv = 1;
	goto cleanup;
    }

    buf = malloc(stat_buf.st_size);

    if ((fd = fopen(req_msg, "r")) == NULL) {
	fprintf(stderr, "%s: failed to open req_msg file %s\n",
		progname, req_msg);
	rv = 1;
	goto cleanup;
    }


    tries = 0;
    length = (size_t) stat_buf.st_size;
    pos_ptr = (char *) buf;
    for (pos = 0; length > 0; length -= pos) {
	pos_ptr += pos;
	if (tries < MAX_IO_TRIES)
	    tries++;
	else {
	    fprintf(stderr, "%s: IO failed while read ap_msg: %s\n",
		    progname, req_msg);
	    rv = 1;
	    goto cleanup;
	}
	pos = fread(pos_ptr, sizeof(char), length, fd);
    }


    packet.length = (size_t) stat_buf.st_size;
    packet.data = (krb5_pointer) buf;

    if ((retval = krb5_rd_req(context, &auth_context, &packet,
			      sprinc, keytab, NULL, &ticket))) {
	com_err(progname, retval, "while reading request");
	rv = 1;
	goto cleanup;
    }
#if 0
    if ((retval =
	 krb5_server_decrypt_ticket_keytab(context, keytab, NULL, ticket)))
    {
	com_err(progname, retval, "while decrypting ticket");
	rv = 1;
	goto cleanup;
    }
#endif


  cleanup:

    if (ticket)
	krb5_free_ticket(context, ticket);

    if (fd)
	fclose(fd);

    if (buf)
	free(buf);

    if (keytab)
	krb5_kt_close(context, keytab);

    if (auth_context)
	krb5_auth_con_free(context, auth_context);

    if (context)
	krb5_free_context(context);

    return rv;
}
