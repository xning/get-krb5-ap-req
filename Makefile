all: get_ap_req get_ap_req.debug proc_ap_req proc_ap_req.debug

get_ap_req: get_ap_req.c
	indent -kr get_ap_req.c
	gcc -O3 -o get_ap_req get_ap_req.c $$(pkg-config --libs --cflags com_err) -I/usr/include/krb5/ -lkrb5

get_ap_req.debug: get_ap_req.c
	indent -kr get_ap_req.c
	gcc -O0 -ggdb -o get_ap_req.debug get_ap_req.c $$(pkg-config --libs --cflags com_err) -I/usr/include/krb5/ -lkrb5


proc_ap_req: proc_ap_req.c
	indent -kr proc_ap_req.c
	gcc -O3 -o proc_ap_req proc_ap_req.c $$(pkg-config --libs --cflags com_err) -I/usr/include/krb5/ -lkrb5

proc_ap_req.debug: proc_ap_req.c
	indent -kr proc_ap_req.c
	gcc -O0 -ggdb -o proc_ap_req.debug proc_ap_req.c $$(pkg-config --libs --cflags com_err) -I/usr/include/krb5/ -lkrb5
