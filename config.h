#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <glib.h>
#include <stdint.h>


enum connection_type {
	CONNECTION_TYPE_LAN,
	CONNECTION_TYPE_WLAN,
	CONNECTION_TYPE_3G,
	CONNECTION_TYPE_UNKNOWN,
};

enum key_type {
	KEY_TYPE_ASCII,
	KEY_TYPE_HEX,
	KEY_TYPE_UNKNOWN,
};

enum auth_type {
	AUTH_TYPE_OPEN,
	AUTH_TYPE_WEPAUTO,
	AUTH_TYPE_WPAPSK,
	AUTH_TYPE_WPA2PSK,
	AUTH_TYPE_UNKNOWN,
};

enum encryption_type {
	ENCRYPTION_TYPE_NONE,
	ENCRYPTION_TYPE_WEP,
	ENCRYPTION_TYPE_AES,
	ENCRYPTION_TYPE_TKIP,
	ENCRYPTION_TYPE_UNKNOWN,
};

enum allocation_type {
        ALLOCATION_TYPE_STATIC,
        ALLOCATION_TYPE_DHCP,
        ALLOCATION_TYPE_UNKNOWN,
};

struct lan_connection {
};

struct wifi_connection {
	char ssid[64];
	char ssid_len;
	char hwaddr[18];
	char key[1024];
	enum key_type key_type;
	enum auth_type auth_type;
	enum encryption_type encryption_type;
};

struct connection {
	enum connection_type connection_type;

	enum allocation_type allocation_type;
	uint32_t ip;
	uint32_t netmask;
	uint32_t gateway;
	uint32_t nameserver1;
	uint32_t nameserver2;

	union {
		struct wifi_connection wlan;
		struct lan_connection lan;
	} phy;
};


gchar *
read_connection(struct connection *conn, xmlNode * a_node);

int
convert_network_configs(char *filename);

void
md5touuid(gchar *input, gchar *output, gint32 output_size);

void
generate_uuid(struct connection *conn, gchar *id, guint32 id_size);

void
write_config_file(struct connection *conn);


#endif //__CONFIG_H__
