#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <libnm-glib/nm-client.h>
#include <libnm-glib/nm-device-wifi.h>
#include <libnm-glib/nm-access-point.h>
#include <nm-utils.h>

#define NM_PATH "/org/freedesktop/NetworkManager"
#define NM_BUS_NAME "org.freedesktop.NetworkManager"
#define NM_IF "org.freedesktop.NetworkManager"
#define NM_IF_DEVICE "org.freedesktop.NetworkManager.Device"
#define NM_IF_IP4CONFIG "org.freedesktop.NetworkManager.IP4Config"


static GMainLoop *loop = NULL;

static guint32
frequency_to_channel(guint32 frequency)
{
	if (frequency == 2412)
		return 1;
	if (frequency == 2417)
		return 2;
	if (frequency == 2422)
		return 3;
	if (frequency == 2427)
		return 4;
	if (frequency == 2432)
		return 5;
	if (frequency == 2437)
		return 6;
	if (frequency == 2442)
		return 7;
	if (frequency == 2447)
		return 8;
	if (frequency == 2452)
		return 9;
	if (frequency == 2457)
		return 10;
	if (frequency == 2462)
		return 11;
	if (frequency == 2467)
		return 12;
	if (frequency == 2472)
		return 13;
	if (frequency == 2484)
		return 14;
	return frequency;
}

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

	g_print("<aps>\n");

	devices = nm_client_get_devices(client);

	for (i=0; devices && i<devices->len; i++)
	{
		NMDevice *dev;
		NMDeviceWifi *wifi;
		const GPtrArray *aps;
		int ap_i;
		const GByteArray *ssid;

		/* Only operate on wifi devices */
		dev = g_ptr_array_index(devices, i);
		if (!NM_IS_DEVICE_WIFI(dev))
			continue;

		wifi = (NMDeviceWifi *)dev;
		aps = nm_device_wifi_get_access_points(wifi);

		for (ap_i=0; aps && ap_i<aps->len; ap_i++)
		{
			NMAccessPoint *ap;
			const char *macaddr;
			guint32 frequency;
			const char *encryption = "NONE";
			const char *auth = "OPEN";
			guint32 flags, wpa_flags, rsn_flags;
			const char *ssid_escaped;

			ap = g_ptr_array_index(aps, ap_i);

			macaddr = nm_access_point_get_hw_address(ap);
			frequency = nm_access_point_get_frequency(ap);

			ssid = nm_access_point_get_ssid (ap);

			flags = nm_access_point_get_flags(ap);
			wpa_flags = nm_access_point_get_wpa_flags(ap);
			rsn_flags = nm_access_point_get_rsn_flags(ap);


			if (   (flags & NM_802_11_AP_FLAGS_PRIVACY)
				&& (wpa_flags == NM_802_11_AP_SEC_NONE)
				&& (rsn_flags == NM_802_11_AP_SEC_NONE))
			{
				encryption = "WEP";
				auth = "WEPAUTO";
			}

			if (wpa_flags != NM_802_11_AP_SEC_NONE)
			{
				if (   (wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X)
				    || (rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X))
					auth = "WPAEAP";
				else
					auth = "WPAPSK";
			}
			if (rsn_flags != NM_802_11_AP_SEC_NONE)
			{
				if (   (wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X)
				    || (rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X))
					auth = "WPA2EAP";
				else
					auth = "WPA2PSK";
			}

			if (        (rsn_flags & NM_802_11_AP_SEC_PAIR_TKIP)
				 || (wpa_flags & NM_802_11_AP_SEC_PAIR_TKIP))
				encryption = "TKIP";
			if (        (rsn_flags & NM_802_11_AP_SEC_PAIR_CCMP)
				 || (wpa_flags & NM_802_11_AP_SEC_PAIR_CCMP))
				encryption = "AES";

			ssid_escaped = "(none)";
			if (ssid)
				if (ssid->data)
					if (ssid->len)
						ssid_escaped = (const char *) nm_utils_escape_ssid (ssid->data, ssid->len);

			g_print(" <ap mode=\"%s\" encryption=\"%s\" channel=\"%d\" hwaddr=\"%s\" auth=\"%s\" linkquality=\"%d\" signalstrength=\"%d\" ssid=\"%s\"/>\n",
				(nm_access_point_get_mode(ap) == NM_802_11_MODE_INFRA) ? "Managed" : "Ad-Hoc",
				encryption,
				frequency_to_channel(frequency),
				macaddr,
				auth,
				nm_access_point_get_strength(ap)/2+50,
				nm_access_point_get_strength(ap)/2+50,
				g_markup_escape_text( ssid_escaped, -1) );
		}
	}
	g_print("</aps>\n");

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
