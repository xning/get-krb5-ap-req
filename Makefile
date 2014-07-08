get_ap_req: get_ap_req.c
	gcc -ggdb -o get_ap_req get_ap_req.c $(pkg-config --libs --cflags krb5 com_err)
