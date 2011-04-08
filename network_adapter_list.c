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


	if (!(client = nm_client_new()))
	{
		g_error("Unable to connect to NetworkManager");
	}

	g_print("<network_adapters>\n");

	devices = nm_client_get_devices(client);

	for (i=0; i<devices->len; i++)
	{
		NMDevice *dev;
		NMDeviceWifi *wifi = NULL;
		NMDeviceEthernet *ether = NULL;
		const char *type;
		const char *hwaddr;

		dev = g_ptr_array_index(devices, i);
		if (NM_IS_DEVICE_WIFI(dev)) {
			wifi = (NMDeviceWifi *)dev;
			type = "wlan";
			hwaddr = nm_device_wifi_get_hw_address (wifi);
		}
		else if (NM_IS_DEVICE_ETHERNET(dev)) {
			ether = (NMDeviceEthernet *)dev;
			type = "lan";
			hwaddr = nm_device_ethernet_get_hw_address (ether);
		}
		else
			continue;

		g_print ("  <adapter if=\"%s\" removable=\"%d\" type=\"%s\" hwaddr=\"%s\" present=\"%d\" />\n",
			nm_device_get_iface (dev),
			1,
			type,
			hwaddr,
			1);
	}

	g_print("</network_adapters>\n");

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
