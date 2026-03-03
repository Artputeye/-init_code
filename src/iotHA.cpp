#include "iotHA.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Diagnostic object define
HASensor ipAddress("ip_address");
HASensor macAddress("mac_address");
HASensor uptime("uptime");
HASensor rssi("rssi");

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Switch object define
HASwitch sw("sw1");

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sensor object define
HASensorNumber temp("sensors");

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Home Assistant setup
void iotHAsetup()
{
    Serial.println("Home assistnt setup: ");

    // set device's details (optional)
    device.setName(DEVICE_NAME);
    device.setSoftwareVersion(D_SoftwareVertion);
    device.setManufacturer(D_Mfac);
    device.setModel(D_Model);
    
    /////////////////////////////////////////////////////////////////////////////
    // Diagnostic info
    ipAddress.setName("IP Address");
    ipAddress.setIcon("mdi:ip-network");
    ipAddress.setStateClass("Diagnostic");

    macAddress.setName("MAC Address");
    macAddress.setIcon("mdi:lan-connect");
    macAddress.setStateClass("Diagnostic");

    uptime.setName("Uptime");
    uptime.setIcon("mdi:timer-sand");
    uptime.setUnitOfMeasurement("seconds");
    uptime.setStateClass("Diagnostic");

    rssi.setName("WiFi RSSI");
    rssi.setIcon("mdi:wifi-strength-4");
    rssi.setUnitOfMeasurement("dBm");
    rssi.setStateClass("Diagnostic");

    // Converse Port char to uint_16
    uint16_t PORT = atoi(MQTT_PORT);

    // IP converse
    byte octets[4];
    if (sscanf(MQTT_ADDR, "%d.%d.%d.%d", &octets[0], &octets[1], &octets[2], &octets[3]) == 4)
    {
        IPAddress ipAddress(octets[0], octets[1], octets[2], octets[3]);
        Serial.print("Parsed IP address: ");
        Serial.println(ipAddress);
    }
    IPAddress MQTT_IP(octets[0], octets[1], octets[2], octets[3]);

    mqtt.begin(MQTT_IP, PORT, MQTT_USERNAME, MQTT_PASSWORD);

    Serial.println("Home assistnt begin: ");
    delay(1000);
}

////////////////////////////////////////////////////////////////////////////////////////
// sent data to MQTT broker
void HA_Diagnostic()
{
    ipAddress.setValue(WiFi.localIP().toString().c_str());
    macAddress.setValue(MacAddr);
    uptime.setValue("10230");
    rssi.setValue("85");
}

void iotHArun(){

}


