#include <stdio.h>
#include <string.h>
#include <nm-remote-settings.h>
#include <nm-client.h>
#include <nm-settings-interface.h>
#include <nm-setting-connection.h>
#include <nm-remote-settings-system.h>
#include <nm-setting-wired.h>
#include <nm-setting-wireless.h>
#include <nm-device-ethernet.h>
#include <nm-setting-pppoe.h>
#include <nm-device-wifi.h>
#include <nm-setting-bluetooth.h>
#include <nm-device-bt.h>
#include <netinet/ether.h>

#include "config.h"

#define NETWORK_CONFIG "/psp/network_config"

static GMainLoop *loop = NULL;
struct start_network_data {
	NMClient *client;
	char uuid[64];
};



static NMConnection *
find_connection(GSList *list, const char *uuid)
{
    NMSettingConnection *s_con;
    NMConnection *connection;
    GSList *iterator;

    iterator = list;
    while (iterator) {
        connection = NM_CONNECTION (iterator->data);
        s_con = (NMSettingConnection *) nm_connection_get_setting (connection, NM_TYPE_SETTING_CONNECTION);
        if (s_con && !strcmp(nm_setting_connection_get_uuid(s_con), uuid))
            return connection;
        iterator = g_slist_next (iterator);
    }
    return NULL;
}


static const char *
active_connection_state_to_string (NMActiveConnectionState state)
{
	switch (state) {
		case NM_ACTIVE_CONNECTION_STATE_ACTIVATING:
			return "activating";
		case NM_ACTIVE_CONNECTION_STATE_ACTIVATED:
			return "activated";
		case NM_ACTIVE_CONNECTION_STATE_UNKNOWN:
		default:
			return "unknown";
	}
}

static gboolean
check_ethernet_compatible (NMDeviceEthernet *device, NMConnection *connection, GError **error)
{
        NMSettingConnection *s_con;
        NMSettingWired *s_wired;
        const char *connection_type;
        gboolean is_pppoe = FALSE;

        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

        s_con = NM_SETTING_CONNECTION (nm_connection_get_setting (connection, NM_TYPE_SETTING_CONNECTION));
        g_assert (s_con);

        connection_type = nm_setting_connection_get_connection_type (s_con);
        if (   strcmp (connection_type, NM_SETTING_WIRED_SETTING_NAME)
            && strcmp (connection_type, NM_SETTING_PPPOE_SETTING_NAME)) {
                g_set_error (error, 0, 0,
                             "The connection was not a wired or PPPoE connection.");
                return FALSE;
        }

        if (!strcmp (connection_type, NM_SETTING_PPPOE_SETTING_NAME))
                is_pppoe = TRUE;

        s_wired = (NMSettingWired *) nm_connection_get_setting (connection, NM_TYPE_SETTING_WIRED);
        /* Wired setting is optional for PPPoE */
        if (!is_pppoe && !s_wired) {
                g_set_error (error, 0, 0,
                             "The connection was not a valid wired connection.");
                return FALSE;
        }

        if (s_wired) {
                const GByteArray *mac;
                const char *device_mac_str;
                struct ether_addr *device_mac;

                device_mac_str = nm_device_ethernet_get_hw_address (device);
                device_mac = ether_aton (device_mac_str);
                if (!device_mac) {
                        g_set_error (error, 0, 0, "Invalid device MAC address.");
                        return FALSE;
                }

                mac = nm_setting_wired_get_mac_address (s_wired);
                if (mac && memcmp (mac->data, device_mac->ether_addr_octet, ETH_ALEN)) {
                        g_set_error (error, 0, 0,
                                     "The connection's MAC address did not match this device.");
                        return FALSE;
                }
        }

        // FIXME: check bitrate against device capabilities

        return TRUE;
}

static gboolean
check_wifi_compatible (NMDeviceWifi *device, NMConnection *connection, GError **error)
{
        NMSettingConnection *s_con;
        NMSettingWireless *s_wireless;

        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

        s_con = NM_SETTING_CONNECTION (nm_connection_get_setting (connection, NM_TYPE_SETTING_CONNECTION));
        g_assert (s_con);

        if (strcmp (nm_setting_connection_get_connection_type (s_con), NM_SETTING_WIRELESS_SETTING_NAME)) {
                g_set_error (error, 0, 0,
                             "The connection was not a WiFi connection.");
                return FALSE;
        }

        s_wireless = NM_SETTING_WIRELESS (nm_connection_get_setting (connection, NM_TYPE_SETTING_WIRELESS));
        if (!s_wireless) {
                g_set_error (error, 0, 0,
                             "The connection was not a valid WiFi connection.");
                return FALSE;
        }

        if (s_wireless) {
                const GByteArray *mac;
                const char *device_mac_str;
                struct ether_addr *device_mac;

                device_mac_str = nm_device_wifi_get_hw_address (device);
                device_mac = ether_aton (device_mac_str);
                if (!device_mac) {
                        g_set_error (error, 0, 0, "Invalid device MAC address.");
                        return FALSE;
                }

                mac = nm_setting_wireless_get_mac_address (s_wireless);
                if (mac && memcmp (mac->data, device_mac->ether_addr_octet, ETH_ALEN)) {
                        g_set_error (error, 0, 0,
                                     "The connection's MAC address did not match this device.");
                        return FALSE;
                }
        }

        // FIXME: check channel/freq/band against bands the hardware supports
        // FIXME: check encryption against device capabilities
        // FIXME: check bitrate against device capabilities

        return TRUE;
}

static gboolean
check_bt_compatible (NMDeviceBt *device, NMConnection *connection, GError **error)
{
        NMSettingConnection *s_con;
        NMSettingBluetooth *s_bt;
        const GByteArray *array;
        char *str;
        const char *device_hw_str;
        int addr_match = FALSE;
        const char *bt_type_str;
        guint32 bt_type, bt_capab;

        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

        s_con = NM_SETTING_CONNECTION (nm_connection_get_setting (connection, NM_TYPE_SETTING_CONNECTION));
        g_assert (s_con);

        if (strcmp (nm_setting_connection_get_connection_type (s_con), NM_SETTING_BLUETOOTH_SETTING_NAME)) {
                g_set_error (error, 0, 0,
                             "The connection was not a Bluetooth connection.");
                return FALSE;
        }

        s_bt = NM_SETTING_BLUETOOTH (nm_connection_get_setting (connection, NM_TYPE_SETTING_BLUETOOTH));
        if (!s_bt) {
                g_set_error (error, 0, 0,
                             "The connection was not a valid Bluetooth connection.");
                return FALSE;
        }

        array = nm_setting_bluetooth_get_bdaddr (s_bt);
        if (!array || (array->len != ETH_ALEN)) {
                g_set_error (error, 0, 0,
                             "The connection did not contain a valid Bluetooth address.");
                return FALSE;
        }

        bt_type_str = nm_setting_bluetooth_get_connection_type (s_bt);
        g_assert (bt_type_str);

        bt_type = NM_BT_CAPABILITY_NONE;
        if (!strcmp (bt_type_str, NM_SETTING_BLUETOOTH_TYPE_DUN))
                bt_type = NM_BT_CAPABILITY_DUN;
        else if (!strcmp (bt_type_str, NM_SETTING_BLUETOOTH_TYPE_PANU))
                bt_type = NM_BT_CAPABILITY_NAP;

        bt_capab = nm_device_bt_get_capabilities (device);
        if (!(bt_type & bt_capab)) {
                g_set_error (error, 0, 0,
                             "The connection was not compatible with the device's capabilities.");
                return FALSE;
        }

        device_hw_str = nm_device_bt_get_hw_address (device);

        str = g_strdup_printf ("%02X:%02X:%02X:%02X:%02X:%02X",
                               array->data[0], array->data[1], array->data[2],
                               array->data[3], array->data[4], array->data[5]);
        addr_match = !strcmp (device_hw_str, str);
        g_free (str);

        return addr_match;
}


static gboolean
nm_device_is_connection_compatible (NMDevice *device, NMConnection *connection, GError **error)
{
	g_return_val_if_fail (NM_IS_DEVICE (device), FALSE);
	g_return_val_if_fail (NM_IS_CONNECTION (connection), FALSE);

	if (NM_IS_DEVICE_ETHERNET (device))
		return check_ethernet_compatible (NM_DEVICE_ETHERNET (device), connection, error);
	else if (NM_IS_DEVICE_WIFI (device))
		return check_wifi_compatible (NM_DEVICE_WIFI (device), connection, error);
	else if (NM_IS_DEVICE_BT (device))
		return check_bt_compatible (NM_DEVICE_BT (device), connection, error);
//      else if (NM_IS_DEVICE_OLPC_MESH (device))
//              return check_olpc_mesh_compatible (NM_DEVICE_OLPC_MESH (device), connection, error);

	g_set_error (error, 0, 0, "unhandled device type '%s'", G_OBJECT_TYPE_NAME (device));
	return FALSE;
}

/**
 * nm_client_get_active_connection_by_path:
 * @client: a #NMClient
 * @object_path: the object path to search for
 *
 * Gets a #NMActiveConnection from a #NMClient.
 *
 * Returns: the #NMActiveConnection for the given @object_path or %NULL if none is found.
 **/
static NMActiveConnection *
nm_client_get_active_connection_by_path (NMClient *client, const char *object_path)
{
        const GPtrArray *actives;
        int i;
        NMActiveConnection *active = NULL;

        g_return_val_if_fail (NM_IS_CLIENT (client), NULL);
        g_return_val_if_fail (object_path, NULL);

        actives = nm_client_get_active_connections (client);
        if (!actives)
                return NULL;

        for (i = 0; i < actives->len; i++) {
                NMActiveConnection *candidate = g_ptr_array_index (actives, i);
                if (!strcmp (nm_object_get_path (NM_OBJECT (candidate)), object_path)) {
                        active = candidate;
                        break;
                }
        }

        return active;
}



static NMDevice *
get_device_for_connection (NMClient *client, NMConnection *connection)
{
	int i;
	const GPtrArray *devices = nm_client_get_devices (client);

	for (i = 0; devices && (i < devices->len); i++) {
		NMDevice *dev = g_ptr_array_index (devices, i);

 		if (nm_device_is_connection_compatible (dev, connection, NULL)) {
			return dev;
		}
	}
	return NULL;
}


static void
active_connection_state_cb (NMActiveConnection *active, GParamSpec *pspec, gpointer user_data)
{
	struct start_network_data *data = (struct start_network_data *)user_data;
        NMActiveConnectionState state;

        state = nm_active_connection_get_state (active);

        g_print ("state: %s\n", active_connection_state_to_string (state));

        if (state == NM_ACTIVE_CONNECTION_STATE_ACTIVATED) {
                g_print("Connection activated\n");
		g_main_loop_quit (loop);
		return;
        } else if (state == NM_ACTIVE_CONNECTION_STATE_UNKNOWN) { 
                g_print("Error: Connection activation failed.\n");
		g_main_loop_quit (loop);
		return;
        }
}



static void
activate_connection_cb (gpointer user_data, const char *path, GError *error)
{
	struct start_network_data *data = (struct start_network_data *)user_data;
	NMActiveConnection *active;
	NMActiveConnectionState state;

	if (error) {
		g_error ("Unable to connect: %s\n", error->message);
		g_main_loop_quit (loop);
		return;
	}

	active = nm_client_get_active_connection_by_path (data->client, path);
	if (!active) {
		g_error ("Unable to obtain active conneciton state for %s\n", path);
		g_main_loop_quit (loop);
		return;
	}

	state = nm_active_connection_get_state (active);
	g_print ("Connection %s state: %s\n", path, active_connection_state_to_string(state));

	/* If we actually connect, then quit */
	if (state == NM_ACTIVE_CONNECTION_STATE_ACTIVATED)
	{
		g_main_loop_quit (loop);
		return;
	}

	g_signal_connect (active, "notify::state", G_CALLBACK (active_connection_state_cb), user_data);
	return;
}

static void
connections_read (NMSettingsInterface *settings, gpointer user_data)
{
    GSList *connections;
    NMConnection *connection;
    NMDevice *dev;
    struct start_network_data *data = (struct start_network_data *)user_data;

    if(!NM_IS_REMOTE_SETTINGS_SYSTEM(settings))
        g_error("Not system settings!  Huh?\n");

    connections = nm_settings_interface_list_connections (settings);
    connection = find_connection(connections, data->uuid);

    fprintf(stderr, "Connection for %s: %p\n", data->uuid, connection);
    if(!connection) {
        g_error("Unable to find connection");
        g_main_loop_quit(loop);
        return;
    }

    dev = get_device_for_connection (data->client, connection);
    if (!dev) {
        g_error ("Unable to find device for connection");
        g_main_loop_quit(loop);
        return;
    }

    nm_client_activate_connection (data->client,
                                   NM_DBUS_SERVICE_SYSTEM_SETTINGS,
                                   nm_connection_get_path (connection),//con_path,
                                   dev,//device,
                                   NULL,//spec_object,
                                   activate_connection_cb,
                                   data);
    return;
}


static void
do_connection (struct start_network_data *data)
{
    DBusGConnection *bus;
    GError *error = NULL;
    NMRemoteSettingsSystem *system_settings;
    gboolean system_settings_running;

    bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
    if (!bus || error) {
        g_error("Unable to connect to system bus");
        goto out;
    }

    system_settings = nm_remote_settings_system_new(bus);
    if(!system_settings) {
        g_error("Unable to get system settings");
        goto out;
    }

    g_object_get (system_settings, NM_REMOTE_SETTINGS_SERVICE_RUNNING, &system_settings_running, NULL);
    if(!system_settings_running) {
        g_error("System settings service is not running");
        goto out;
    }


    g_signal_connect (system_settings, NM_SETTINGS_INTERFACE_CONNECTIONS_READ,
                                       G_CALLBACK (connections_read), data);

out:
    if(bus)
        dbus_g_connection_unref(bus);
    return;
}




static gboolean
glib_main (gpointer data)
{
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
    struct connection conn;
    static struct start_network_data start_network_data;

    if (!(start_network_data.client = nm_client_new()))
    {
        g_error("Unable to connect to NetworkManager");
    }


    /*parse the file and get the DOM */
    if( NULL == (doc = xmlReadFile(NETWORK_CONFIG, NULL, 0))) {
        printf("error: Could not parse " NETWORK_CONFIG "\n");
        return 1;
    }

    /*Get the root element node */
    root_element = xmlDocGetRootElement(doc);
    if (root_element->type == XML_ELEMENT_NODE && !strcmp((char *)root_element->name, "configuration")) {
        read_connection(&conn, root_element);
    }
    else {
        fprintf(stderr, "error: Invalid " NETWORK_CONFIG " file: No <configuration/> root node\n");
        return 1;
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();

    /* Generate the UUID from the connection information */
    generate_uuid(&conn, start_network_data.uuid, sizeof(start_network_data.uuid));


    do_connection(&start_network_data);

    return FALSE;
}


int
main(int argc, char **argv)
{
    /* glib overhead */
    g_type_init();
    g_idle_add (glib_main, NULL);
    loop = g_main_loop_new (NULL, FALSE);  /* create main loop */
    g_main_loop_run (loop);                /* run main loop */
    return 0;
}
