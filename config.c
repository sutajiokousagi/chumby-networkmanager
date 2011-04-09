/**
 * section: Tree
 * synopsis: Navigates a tree to print element names
 * purpose: Parse a file to a tree, use xmlDocGetRootElement() to
 *          get the root element, then walk the document and print
 *          all the element name in document order.
 * usage: tree1 filename_or_URL
 * test: tree1 test2.xml > tree1.tmp ; diff tree1.tmp tree1.res ; rm tree1.tmp
 * author: Dodji Seketeli
 * copy: see Copyright for the status of this software.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <glib.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <netinet/ether.h>

#include "config.h"

#define SYSTEM_CONNECTIONS_PATH "/etc/NetworkManager/system-connections"

void
md5touuid(gchar *input, gchar *output, gint32 output_size)
{
	int input_i, output_i;
	bzero(output, output_size);

	for(input_i = output_i = 0; output_i < output_size-1; output_i++, input_i++)
	{
		output[output_i] = input[input_i];
		if(input_i == 7 || input_i == 11 || input_i == 15 || input_i == 19)
		{
			output[++output_i] = '-';
		}
	}
}

static void 
strtoip(char *s, uint32_t *addr) 
{ 
	*addr = inet_addr(s);
}

void
generate_uuid(struct connection *conn, gchar *id, guint32 id_size)
{
    /* Make the UUID just be a hash of the SSID */
    gchar *raw_id;
    raw_id = g_compute_checksum_for_data(G_CHECKSUM_MD5, (guchar *)conn, sizeof(*conn));
    md5touuid(raw_id, id, id_size);
}

#if 0
static void
print_ssid(FILE *output, struct connection *conn)
{
	int i;
	int printable = TRUE;
	/* Check whether each byte is printable.  If not, we have to use an
	 * integer list, otherwise we can just use a string.
	 */
	for (i = 0; i < conn->phy.wlan.ssid_len; i++) {
		char c = conn->phy.wlan.ssid[i] & 0xFF;
		if (!isprint (c)) {
			printable = FALSE;
			break;
		}
	}

//	if (printable)
//		fprintf(output, "%s\n", conn->phy.wlan.ssid);
//	else
	{
		for (i=0; i < conn->phy.wlan.ssid_len; i++)
			fprintf(output, "%d;", conn->phy.wlan.ssid[i]);
		fprintf(output, "\n");
	}
}
#endif

/*
 *To compile this file using gcc you can type
 *gcc `xml2-config --cflags --libs` -o xmlexample libxml2-example.c
 */

/**
 * print_element_names:
 * @a_node: the initial xml node to consider.
 *
 * Prints the names of the all the xml elements
 * that are siblings or children of a given xml node.
 */
gboolean
read_connection(struct connection *conn, xmlNode * a_node)
{
    bzero(conn, sizeof(*conn));
    xmlAttr *properties = a_node->properties;
    for(properties = a_node->properties; properties; properties = properties->next) {
        char *name = (char *)properties->name;
        char *value = (char *)properties->children->content;

        if(!strcmp(name, "type")) {
            if(!strcmp(value, "wlan"))
                conn->connection_type = CONNECTION_TYPE_WLAN;
            else if(!strcmp(value, "lan"))
                conn->connection_type = CONNECTION_TYPE_LAN;
            else {
                fprintf(stderr, "Unrecognized connection type: %s\n", value);
                return FALSE;
            }
        }

        else if(!strcmp(name, "hwaddr"))
            strncpy(conn->phy.wlan.hwaddr, value, sizeof(conn->phy.wlan.hwaddr)-1);

        else if(!strcmp(name, "ssid")) {
            strncpy(conn->phy.wlan.ssid, value, sizeof(conn->phy.wlan.ssid)-1);
            conn->phy.wlan.ssid_len = xmlUTF8Size(properties->name);
        }

        else if(!strcmp(name, "auth")) {
            if(!strcmp(value, "OPEN"))
                conn->phy.wlan.auth_type = AUTH_TYPE_OPEN;
            else if(!strcmp(value, "WEPAUTO"))
                conn->phy.wlan.auth_type = AUTH_TYPE_WEPAUTO;
            else if(!strcmp(value, "WPAPSK"))
                conn->phy.wlan.auth_type = AUTH_TYPE_WPAPSK;
            else if(!strcmp(value, "WPA2PSK"))
                conn->phy.wlan.auth_type = AUTH_TYPE_WPA2PSK;
            else {
                fprintf(stderr, "Unrecognized auth: %s\n", value);
                return FALSE;
            }
        }

        else if(!strcmp(name, "encryption")) {
            if(!strcmp(value, "NONE"))
                conn->phy.wlan.encryption_type = ENCRYPTION_TYPE_NONE;
            else if(!strcmp(value, "WEP"))
                conn->phy.wlan.encryption_type = ENCRYPTION_TYPE_WEP;
            else if(!strcmp(value, "TKIP"))
                conn->phy.wlan.encryption_type = ENCRYPTION_TYPE_TKIP;
            else if(!strcmp(value, "AES"))
                conn->phy.wlan.encryption_type = ENCRYPTION_TYPE_AES;
            else {
                fprintf(stderr, "Unrecognized encryption: %s\n", value);
                return FALSE;
            }
        }

        else if(!strcmp(name, "encoding")) {
            if(!strcmp(value, "ascii"))
                conn->phy.wlan.key_type = KEY_TYPE_ASCII;
            else if(!strcmp(value, "hex"))
                conn->phy.wlan.key_type = KEY_TYPE_HEX;
            else {
                fprintf(stderr, "Unrecognized encoding type: %s\n", value);
                return FALSE;
            }
        }

        else if(!strcmp(name, "key"))
            strncpy(conn->phy.wlan.key, value, sizeof(conn->phy.wlan.key)-1);

        else if(!strcmp(name, "allocation")) {
            if(!strcmp(value, "static"))
                conn->allocation_type = ALLOCATION_TYPE_STATIC;
            else if(!strcmp(value, "dhcp"))
                conn->allocation_type = ALLOCATION_TYPE_DHCP;
            else {
                fprintf(stderr, "Unrecognized allocation: %s\n", value);
                return FALSE;
            }
        }

        else if(!strcmp(name, "ip"))
            strtoip(value, &conn->ip);
        else if(!strcmp(name, "netmask"))
            strtoip(value, &conn->netmask);
        else if(!strcmp(name, "gateway"))
            strtoip(value, &conn->gateway);
        else if(!strcmp(name, "nameserver1"))
            strtoip(value, &conn->nameserver1);
        else if(!strcmp(name, "nameserver2"))
            strtoip(value, &conn->nameserver2);
        else if(!strcmp(name, "username"))
            ; // Ignore
        else {
            fprintf(stderr, "Unrecognized field: %s\n", name);
        }
    }

    return TRUE;
}

#if 0
void
write_config_file(struct connection *conn)
{
    gchar id[40];
    gchar filename[PATH_MAX];
    FILE *output;

    generate_uuid(conn, id, sizeof(id));

    snprintf(filename, sizeof(filename), "%s/%s.conf", SYSTEM_CONNECTIONS_PATH, id);
    umask(0077);
    output = fopen(filename, "w");
    if(!output) {
        perror("Unable to open connections file for writing");
        return;
    }


    fprintf(output, "[connection]\n");
    if(conn->connection_type == CONNECTION_TYPE_WLAN)
        fprintf(output, "id=%s\n", conn->phy.wlan.ssid);
    else
        fprintf(output, "id=lan\n");
    fprintf(output, "uuid=%s\n", id);

    if(conn->connection_type == CONNECTION_TYPE_WLAN) {
        fprintf(output, "type=%s\n", "802-11-wireless");
        fprintf(output, "\n");
        fprintf(output, "[802-11-wireless]\n");
        fprintf(output, "ssid=");
        print_ssid(output, conn);
        fprintf(output, "\n");

        fprintf(output, "\n");

        fprintf(output, "[802-11-wireless-security]\n");
        if(conn->phy.wlan.auth_type == AUTH_TYPE_OPEN || conn->phy.wlan.auth_type == AUTH_TYPE_WEPAUTO) {
            fprintf(output, "key-mgmt=none\n");
        }
        else if(conn->phy.wlan.auth_type == AUTH_TYPE_WPAPSK || conn->phy.wlan.auth_type == AUTH_TYPE_WPA2PSK) {
            fprintf(output, "key-mgmt=none\n");
        }

        if(conn->phy.wlan.encryption_type == ENCRYPTION_TYPE_WEP) {
            fprintf(output, "wep-key0=%s\n", conn->phy.wlan.key);
            fprintf(output, "wep-key-type=1\n");
        }
        else if(conn->phy.wlan.encryption_type == ENCRYPTION_TYPE_AES || conn->phy.wlan.encryption_type == ENCRYPTION_TYPE_TKIP) {
            fprintf(output, "key-mgmt=wpa-psk\n");
            fprintf(output, "psk=%s\n", conn->phy.wlan.key);
        }
    }

    else if(conn->connection_type == CONNECTION_TYPE_LAN) {
        fprintf(output, "type=%s\n", "802-3-ethernet");
        fprintf(output, "\n");
        fprintf(output, "[802-3-ethernet]\n");
    }



    fprintf(output, "\n");
    fprintf(output, "[ipv4]\n");
    conn->allocation_type = ALLOCATION_TYPE_DHCP;
    if(conn->allocation_type == ALLOCATION_TYPE_DHCP) {
        fprintf(output, "method=auto\n");
    }
    else {
        int netmask;
        fprintf(output, "method=manual\n");
	for(netmask=31; netmask>=0; netmask--)
            if(conn->netmask & (1<<netmask))
                break;
        fprintf(output, "addresses=%u,%u,%u\n", conn->ip, netmask+1, conn->gateway);
        fprintf(output, "dns=%u,%u\n", conn->nameserver1, conn->nameserver2);
    }
    fclose(output);
}
#endif

#if 0
/**
 * Convert the network_configs file to NetworkManager's keyfile
 */
int
convert_network_configs(char *filename)
{

    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
    xmlNode *cur_node;

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    /*parse the file and get the DOM */
    doc = xmlReadFile(filename, NULL, 0);

    if (doc == NULL) {
        printf("error: could not parse file %s\n", filename);
        return 1;
    }

    /*Get the root element node */
    root_element = xmlDocGetRootElement(doc);
    for (cur_node = root_element->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE && !strcmp((char *)cur_node->name, "configuration")) {
            struct connection conn;
            read_connection(&conn, cur_node);
            write_config_file(&conn);
        }
    }


    /*free the document */
    xmlFreeDoc(doc);

    /*
     *Free the global variables that may
     *have been allocated by the parser.
     */
    xmlCleanupParser();

    return 0;
}
#endif
