#include "GoveeBTh5179.h"
#include "NimBLEDevice.h"

NimBLEScan *pBLEScan;
NimBLEUUID serviceUuid("180a"); // Govee 5179 service UUID
long nextScanTime;
const long scanInterval = 60 * 1000;
double temp = 0.;
double humidity = 0.;
int battery = 0;
void (*eventlistner)(double temp,double hum,int bat);
bool enable = false;

struct goveebtdata
{
  char ident[6];
  uint16_t temp;
  uint16_t humidity;
  uint8_t bat;
} btdata;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{

  void onDiscovered(BLEAdvertisedDevice *advertisedDevice)
  {
    // log_i("Advertised Device: %s \n", advertisedDevice->toString().c_str());
  }

  /*
  [ 68866][I][main.cpp:43] onResult(): Advertised Device: Name: Govee_H5179_9C4D,, manufacturer data: 0188ec000101760c9a1552, serviceUUID: 0x180a

  [ 68882][I][main.cpp:63] onResult(): Govee_H5179_9C4D
  [ 68887][I][main.cpp:64] onResult():  2.57 31.90 154
  */
  // Called when BLE scan sees BLE advertisement.
  //                    head     hum   tmp  ka   bat
  // manufacturer data: 0188ec00 0101  6c0c ae15 52, serviceUUID: 0x180a
  //                    0188EC00 0101  220B 2C10 53
  //                    0188EC00 0101  180B 2C10 52
  //                    181201121114020
  void onResult(BLEAdvertisedDevice *advertisedDevice)
  {

    // check for Govee 5074 service UUID
    if (advertisedDevice->getServiceUUID() == serviceUuid)
    {
      // log_i("Advertised Device: %s \n", advertisedDevice->toString().c_str());
      //  Desired Govee advert will have 11 byte mfg. data length & leading bytes0x01 0x88 0xec
      if ((advertisedDevice->getManufacturerData().length() == 11) &&
          ((byte)advertisedDevice->getManufacturerData().data()[1] == 0x88) &&
          ((byte)advertisedDevice->getManufacturerData().data()[2] == 0xec))
      {
        char buf[advertisedDevice->getManufacturerData().length()];
        for (int i = 0; i < advertisedDevice->getManufacturerData().length(); i++)
        {
          buf[i] = advertisedDevice->getManufacturerData()[i];
        }
        goveebtdata *data = (goveebtdata *)buf;
        temp = ((double)data->temp) / 100.;
        humidity = ((double)data->humidity) / 100.;
        battery = data->bat;
        log_i("%s", advertisedDevice->getName().c_str());
        log_i("%i %i %i", data->temp, data->humidity, data->bat);
        if(eventlistner != nullptr)
            eventlistner(temp,humidity,battery);
      }
    }
    else
    {
      pBLEScan->erase(advertisedDevice->getAddress());
    }
  }
};
MyAdvertisedDeviceCallbacks *btcallback;

void GoveeBTh5179_setup()
{
    NimBLEDevice::setScanDuplicateCacheSize(10);
    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan(); // create new scan
    // Set the callback for when devices are discovered, include duplicates.
    btcallback = new MyAdvertisedDeviceCallbacks();
    log_i("btcallback  %i", btcallback);
    pBLEScan->setScanCallbacks(btcallback, true);
    pBLEScan->setActiveScan(true); // Set active scanning, this will get more data from the advertiser.
    pBLEScan->setInterval(1000);   // How often the scan occurs / switches channels; in milliseconds,
    pBLEScan->setWindow(900);      // How long to scan during the interval; in milliseconds.
    pBLEScan->setMaxResults(0);    // do not store the scan results, use callback only.
}

void GoveeBTh5179_loop()
{
    if (enable && nextScanTime <= millis() && !pBLEScan->isScanning())
    {
      nextScanTime += scanInterval;
      log_i("Restarting BLE scan");
      pBLEScan->start(1000, false);
    }
}

double GoveeBTh5179_getTemperature()
{
    return temp;
}

double GoveeBTh5179_getHumidity()
{
    return humidity;
}

int GoveeBTh5179_getBattery()
{
    return battery;
}

void GoveeBTh5179_setEventListner(void func(double temp, double hum, int bat))
{
    eventlistner = func;
}

void GoveeBTh5179_enable(bool en)
{
    enable = en;
}

bool GoveeBTh5179_isEnable()
{
    return enable;
}
