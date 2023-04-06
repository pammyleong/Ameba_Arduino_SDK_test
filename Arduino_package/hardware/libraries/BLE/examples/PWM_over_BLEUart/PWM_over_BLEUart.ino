/*

 Example guide:
 https://www.amebaiot.com/en/amebapro2-amb82-mini-arduino-ble-pwm/
 */

#include "BLEDevice.h"

// Choose the RGB LED type
//#define AnodeRGB
#define CathodeRGB

// Choose 3 PWM pins for RGB LED pins refer to the pinmap of using board
#define LED_RED      1
#define LED_GRN      2
#define LED_BLU      3

#define UART_SERVICE_UUID      "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define STRING_BUF_SIZE 100

BLEService UartService(UART_SERVICE_UUID);
BLECharacteristic Rx(CHARACTERISTIC_UUID_RX);
BLECharacteristic Tx(CHARACTERISTIC_UUID_TX);
BLEAdvertData advdata;
BLEAdvertData scndata;
bool notify = false;

void readCB (BLECharacteristic* chr, uint8_t connID) {
    printf("Characteristic %s read by connection %d \n", chr->getUUID().str(), connID);
}

void writeCB (BLECharacteristic* chr, uint8_t connID) {
    printf("Characteristic %s write by connection %d :\n", chr->getUUID().str(), connID);
    uint16_t datalen = chr->getDataLen();
    if (datalen > 0) {
        if (chr->readData8() == '!') {
            uint8_t command[datalen];
            chr->getData(command, datalen);
            if (command[1] == 'C') {
                // print hex
                printf("Color command R = %x G = %x B = %x \n", command[2], command[3], command[4]);
                // print decimal
                //printf("Color command R = %d G = %d B = %d \n", command[2], command[3], command[4]);
#if defined(CathodeRGB)
                analogWrite(LED_RED, command[2]);
                analogWrite(LED_GRN, command[3]);
                analogWrite(LED_BLU, command[4]);
#elif defined(AnodeRGB)
                analogWrite(LED_RED, (255 - command[2]));
                analogWrite(LED_GRN, (255 - command[3]));
                analogWrite(LED_BLU, (255 - command[4]));
#else
                printf("Error, please choose the RGB LED type \n");
#endif
            }
        } else {
            Serial.print("Received string: ");
            Serial.print(chr->readString());
            Serial.println();
        }
    }
}

void notifCB (BLECharacteristic* chr, uint8_t connID, uint16_t cccd) {
    if (cccd & GATT_CLIENT_CHAR_CONFIG_NOTIFY) {
        printf("Notifications enabled on Characteristic %s for connection %d \n", chr->getUUID().str(), connID);
        notify = true;
    } else {
        printf("Notifications disabled on Characteristic %s for connection %d \n", chr->getUUID().str(), connID);
        notify = false;
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GRN, OUTPUT);
    pinMode(LED_BLU, OUTPUT);

    analogWrite(LED_RED, 255);
    analogWrite(LED_GRN, 255);
    analogWrite(LED_BLU, 255);
    
    advdata.addFlags(GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED);
    advdata.addCompleteName("AMEBA_BLE_DEV");
    scndata.addCompleteServices(BLEUUID(UART_SERVICE_UUID));

    Rx.setWriteProperty(true);
    Rx.setWritePermissions(GATT_PERM_WRITE);
    Rx.setWriteCallback(writeCB);
    Rx.setBufferLen(STRING_BUF_SIZE);
    Tx.setReadProperty(true);
    Tx.setReadPermissions(GATT_PERM_READ);
    Tx.setReadCallback(readCB);
    Tx.setNotifyProperty(true);
    Tx.setCCCDCallback(notifCB);
    Tx.setBufferLen(STRING_BUF_SIZE);

    UartService.addCharacteristic(Rx);
    UartService.addCharacteristic(Tx);

    BLE.init();
    BLE.configAdvert()->setAdvData(advdata);
    BLE.configAdvert()->setScanRspData(scndata);
    BLE.configServer(1);
    BLE.addService(UartService);

    BLE.beginPeripheral();
}

void loop() {
    if (Serial.available()) {
        Tx.writeString(Serial.readString());
        if (BLE.connected(0) && notify) {
            Tx.notify(0);
        }
    }
    delay(100);
}