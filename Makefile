all: start_network ap_scan network_adapter_list macgen network_status
clean: ap_scan_clean start_network_clean network_adapter_list_clean macgen_clean network_status_clean

MYCFLAGS+=`pkg-config libxml-2.0 --cflags` `pkg-config glib-2.0 --cflags --libs` `pkg-config libnm-glib --cflags` `pkg-config NetworkManager --cflags` `pkg-config dbus-1 --cflags` -Wall
MYLDFLAGS+=`pkg-config libxml-2.0 --libs` `pkg-config glib-2.0 --libs` `pkg-config libnm-glib --libs` `pkg-config NetworkManager --libs` `pkg-config dbus-1 --libs`

start_network_clean:
	rm -f config.o start_network.o start_network

start_network: config.o start_network.o
	$(CC) config.o start_network.o -o start_network $(MYLDFLAGS) $(LDFLAGS)

config.o: config.c config.h
	$(CC) config.c -c -o config.o $(MYCFLAGS) $(CFLAGS)

start_network.o: start_network.c config.h
	$(CC) start_network.c -c -o start_network.o $(MYCFLAGS) $(CFLAGS)


ap_scan_clean:
	rm -f ap_scan

ap_scan: ap_scan.c
	$(CC)  ap_scan.c -o ap_scan $(MYCFLAGS) $(MYLDFLAGS) $(CFLAGS) $(LDFLAGS)



network_adapter_list_clean:
	rm -f network_adapter_list.sh

network_adapter_list: network_adapter_list.c
	$(CC)  network_adapter_list.c -o network_adapter_list.sh $(MYCFLAGS) $(MYLDFLAGS) $(CFLAGS) $(LDFLAGS)


macgen_clean:
	rm -f macgen.sh

macgen: macgen.c
	$(CC)  macgen.c -o macgen.sh $(MYCFLAGS) $(MYLDFLAGS) $(CFLAGS) $(LDFLAGS)



network_status_clean:
	rm -f network_status.sh

network_status: network_status.c
	$(CC)  network_status.c -o network_status.sh $(MYCFLAGS) $(MYLDFLAGS) $(CFLAGS) $(LDFLAGS)

