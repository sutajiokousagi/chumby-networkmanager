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



gboolean
signal_strength (gpointer data, gchar *errormessage)
{
	NMClient *client;
	const GPtrArray *connections;
	int i;
	int printed = 0;


	client = (NMClient *)data;
	if (!client)
	{
		if (!(client = nm_client_new()))
		{
			g_print ("<wifi connected=\"0\" />\n");
			if (loop)
				g_main_loop_quit (loop);
			return FALSE;
		}
	}

	connections = nm_client_get_active_connections (client);

	if (connections)
	{
		for (i=0; connections && !printed && i<connections->len; i++)
		{
			int j;
			NMActiveConnection *con;
			const GPtrArray *devices;

			con = g_ptr_array_index(connections, i);
			devices = nm_active_connection_get_devices (con);

			for (j=0; !printed && j<devices->len; j++)
			{
				NMDevice *dev;
				NMAccessPoint *ap;

				dev = g_ptr_array_index(devices, j);

				if (!NM_IS_DEVICE_WIFI(dev))
					continue;

				ap = nm_device_wifi_get_active_access_point(NM_DEVICE_WIFI(dev));
				if (!ap)
					continue;

				g_print ("<wifi connected=\"1\" signalstrength=\"%d\" />\n",
					 nm_access_point_get_strength(ap));
				printed = 1;
			}
		}
	}

	if (!printed)
		g_print ("<wifi connected=\"0\" />\n");

	if (loop)
		g_main_loop_quit (loop);
	return FALSE;
}

gboolean
network_status_wrapper (gpointer data)
{
	return signal_strength (data, NULL);
}


int
main (int argc, char **argv)
{
	/* glib overhead */
	g_type_init();
	g_idle_add (network_status_wrapper, NULL);
	loop = g_main_loop_new (NULL, FALSE);  /* create main loop */
	g_main_loop_run (loop);                /* run main loop */
	return 0;
}
