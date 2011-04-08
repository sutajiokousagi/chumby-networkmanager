#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <libnm-glib/nm-client.h>
#include <libnm-glib/nm-device-wifi.h>
#include <libnm-glib/nm-device-ethernet.h>


static GMainLoop *loop = NULL;


static gboolean
glib_main (gpointer data)
{
	NMClient *client;
	const GPtrArray *devices;
	int i;
	int pass;


	if (!(client = nm_client_new()))
	{
		g_error("Unable to connect to NetworkManager");
	}

	devices = nm_client_get_devices(client);

	/* Three passes:
		0) Look for an ACTIVE connection
		1) Look for a WIRELESS connection
		2) Look for any connection
	*/
	for (pass=0; pass<3; pass++)
	{
		for (i=0; i<devices->len; i++)
		{
			NMDevice *dev;
			const char *hwaddr;

			dev = g_ptr_array_index(devices, i);
			if (NM_IS_DEVICE_WIFI(dev))
				hwaddr = nm_device_wifi_get_hw_address ((NMDeviceWifi *)dev);
			else if (NM_IS_DEVICE_ETHERNET(dev))
				hwaddr = nm_device_ethernet_get_hw_address ((NMDeviceEthernet *)dev);
			else
				continue;

			/* If we hit an active connection, print and bail */
			if ( (pass == 0 && (nm_device_get_state(dev) == NM_DEVICE_STATE_ACTIVATED))
			  || (pass == 1 && NM_IS_DEVICE_WIFI(dev))
			  || (pass == 2))
				g_print ("%s\n", hwaddr);
				g_main_loop_quit (loop);
				return FALSE;
		}
	}

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
