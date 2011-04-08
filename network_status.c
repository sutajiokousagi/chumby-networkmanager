#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <nm-utils.h>
#include <nm-setting-ip4-config.h>
#include <libnm-glib/nm-ip4-config.h>
#include <libnm-glib/nm-client.h>
#include <libnm-glib/nm-device-wifi.h>
#include <libnm-glib/nm-device-ethernet.h>
#include <libnm-glib/nm-device-ethernet.h>


static GMainLoop *loop = NULL;


NMIP4Address *nm_ip4_config_get_address (NMIP4Config *config, guint i)
{
	g_return_val_if_fail (NM_IS_IP4_CONFIG (config), NULL);
	GSList *addresses = (GSList *) nm_ip4_config_get_addresses (config);
	return (NMIP4Address *) g_slist_nth_data (addresses, i);
}

NMIP4Route *
nm_ip4_config_get_route (NMIP4Config *config, guint i)
{
        g_return_val_if_fail (NM_IS_IP4_CONFIG (config), NULL);
        GSList *routes = (GSList *) nm_ip4_config_get_routes (config);
        return (NMIP4Route *) g_slist_nth_data (routes, i);
}




static void
print_device (NMClient *client, NMActiveConnection *con, NMDevice *dev)
{
	const char *state;
	const char *link;
	NMIP4Config *ipv4;

	if (nm_device_get_state (dev) == NM_DEVICE_STATE_ACTIVATED)
	{
		state = "true";
		link = "true";
	}
	else {
		state = "false";
		link = "false";
	}

	ipv4 = nm_device_get_ip4_config (dev);
	
	if (ipv4)
	{
		NMIP4Address *ipv4_addr;
		struct sockaddr_in sa;
		int dns_i;
		const GArray *dns;
		char ip_s[INET_ADDRSTRLEN];
		char broadcast_s[INET_ADDRSTRLEN];
		char netmask_s[INET_ADDRSTRLEN];
		char gateway_s[INET_ADDRSTRLEN];

                guint32 hostmask, network, bcast, netmask;


		ipv4_addr = nm_ip4_config_get_address (ipv4, 0);

                netmask = nm_utils_ip4_prefix_to_netmask (nm_ip4_address_get_prefix (ipv4_addr));
                network = ntohl (nm_ip4_address_get_address (ipv4_addr)) & ntohl (netmask);
                hostmask = ~ntohl (netmask);
                bcast = htonl (network | hostmask);

		sa.sin_addr.s_addr = nm_ip4_address_get_address (ipv4_addr);
		inet_ntop(AF_INET, &(sa.sin_addr), ip_s, INET_ADDRSTRLEN);

		sa.sin_addr.s_addr = nm_ip4_address_get_gateway (ipv4_addr);
		inet_ntop(AF_INET, &(sa.sin_addr), gateway_s, INET_ADDRSTRLEN);

		sa.sin_addr.s_addr = netmask;
		inet_ntop(AF_INET, &(sa.sin_addr), netmask_s, INET_ADDRSTRLEN);

		sa.sin_addr.s_addr = bcast;
		inet_ntop(AF_INET, &(sa.sin_addr), broadcast_s, INET_ADDRSTRLEN);

		g_print ("\t<interface if=\"%s\" up=\"%s\" link=\"%s\" ip=\"%s\" broadcast=\"%s\" netmask=\"%s\" gateway=\"%s\"",
			nm_device_get_iface (dev),
			state,
			link,
			ip_s,
			broadcast_s,
			netmask_s,
			gateway_s);

		dns = nm_ip4_config_get_nameservers (ipv4);
		for (dns_i=0; dns_i<dns->len; dns_i++)
		{
			char dns_s[INET_ADDRSTRLEN];
			guint32 *addrs = (guint32 *)dns->data;
			sa.sin_addr.s_addr = addrs[dns_i];
			inet_ntop(AF_INET, &(sa.sin_addr), dns_s, INET_ADDRSTRLEN);
			g_print (" nameserver%d=\"%s\"", dns_i+1, dns_s);
		}
		g_print (">\n");
	}
	else {
		g_print ("\t<interface if=\"%s\" up=\"%s\" link=\"%s\">\n",
			nm_device_get_iface (dev),
			state,
			link);
		g_print ("\t\t<error>Failed to obtain IP address</error>\n");
		g_print ("\t\t<error>Interface is down</error>\n");
	}
	g_print ("\t</interface>\n");
//gateway=\"%s\" nameserver1=\"%s\">\n");
}
		


static gboolean
glib_main (gpointer data)
{
	NMClient *client;
	const GPtrArray *connections;
	int i;
	int printed = 0;


	if (!(client = nm_client_new()))
	{
		g_print ("<network>\n\t<interface>\n\t\t<error>Unable to connect to NetworkManager</error>\n\t</interface>\n</network>\n");
		g_main_loop_quit (loop);
		return FALSE;
	}

	g_print ("<network>\n");

	connections = nm_client_get_active_connections (client);

	for (i=0; !printed && i<connections->len; i++)
	{
		int j;
		NMActiveConnection *con;
		const GPtrArray *devices;

		con = g_ptr_array_index(connections, i);
		devices = nm_active_connection_get_devices (con);

		for (j=0; !printed && j<devices->len; j++)
		{
			NMDevice *dev;
			dev = g_ptr_array_index(devices, j);

			print_device (client, con, dev);
			printed = 1;
		}
	}

	if (!printed)
	{
		g_print ("\t<interface>\n\t\t<error>Device not found</error>\n\t</interface>\n");
	}

	g_print ("</network>\n");

	g_main_loop_quit (loop);
	return FALSE;
}

int
main (int argc, char **argv)
{
	/* glib overhead */
	g_type_init();
	g_idle_add (glib_main, NULL);
	loop = g_main_loop_new (NULL, FALSE);  /* create main loop */
	g_main_loop_run (loop);                /* run main loop */
	return 0;
}
