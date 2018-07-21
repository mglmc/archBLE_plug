#include "mbed.h"
#include "ble/BLE.h"

DigitalOut led(p13, 0);
AnalogIn pinAnalogIn (p1);

uint16_t customServiceUUID  = 0xB000;
uint16_t readCharUUID       = 0xB001;
uint16_t writeCharUUID      = 0xB002;
uint16_t showStateUUID      = 0xB003;

const static char     DEVICE_NAME[]        = "SmartPlug"; // change this
static const uint16_t uuid16_list[]        = {0xFFFF}; //Custom UUID, FFFF is reserved for development

float mVperAmp = 0.141; // use 100 for 20A Module and 66 for 
                    //30A Module sensibility 0.141 V/A
Timer t;

/* Set Up custom Characteristics */
static uint8_t readValue[10] = {0};
ReadOnlyArrayGattCharacteristic<uint8_t, sizeof(readValue)> readChar(readCharUUID, readValue);

static uint8_t writeValue[10] = {0};
WriteOnlyArrayGattCharacteristic<uint8_t, sizeof(writeValue)> writeChar(writeCharUUID, writeValue);

static uint8_t showStateValue[10] = {0};
ReadOnlyArrayGattCharacteristic<uint8_t, sizeof(showStateValue)> showStateChar(showStateUUID, showStateValue, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);


/* Set up custom service */
GattCharacteristic *characteristics[] = {&readChar, &writeChar, &showStateChar };
GattService        customService(customServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));


float getVPP()
{
    float result;

    float valuePinAnalogfloat;
    //uint8_t ui8valuePinAnalog; //value read from the sensor     
    float maxValue = 0;          // store max value here
    float minValue = 1;          // store min value here
    
    float begin, end;
    t.start();
    t.reset();
    
    begin = t.read_us();
    end = begin;
    int i = 0;
    while((end - begin) < 1000) //sample for 1 Sec
    {
        valuePinAnalogfloat = (pinAnalogIn.read());
        //printf("Value of ui8valuePinAnalog is %f \n\r", valuePinAnalogfloat);
        //ui8valuePinAnalog = valuePinAnalogfloat*1024;
        //printf("Value of ui8valuePinAnalog is %i \n\r", ui8valuePinAnalog); 
        // see if you have a new maxValue
        if (valuePinAnalogfloat > maxValue) 
        {
            /*record the maximum sensor value*/
            maxValue = valuePinAnalogfloat;
        }
        if (valuePinAnalogfloat < minValue) 
        {
           /*record the maximum sensor value*/
           minValue = valuePinAnalogfloat;
        }
        i++;
        end = t.read_ms();
       
    }
       // Subtract min from max
   result = ((maxValue - minValue)*3.3);
   printf("Result is result: %f, maxValue: %f minValue: %f muestras: %i \n\r", result, maxValue, minValue, i);
      
   return (result);
}

/*
 *  Restart advertising when phone app disconnects
*/
void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *)
{
    BLE::Instance(BLE::DEFAULT_INSTANCE).gap().startAdvertising();
}

/*
 *  Handle writes to writeCharacteristic
*/
void writeCharCallback(const GattWriteCallbackParams *params)
{
    /* Check to see what characteristic was written, by handle */
    if(params->handle == writeChar.getValueHandle()) {
        /* toggle LED if only 1 byte is written */
        led = params->data[0];
        (params->data[0] == 0x00) ? printf("led off\n\r") : printf("led on\n\r"); // print led toggle
        /* Update the readChar with the value of writeChar */
        BLE::Instance(BLE::DEFAULT_INSTANCE).gattServer().write(readChar.getValueHandle(), params->data, params->len);
    }
}

/*
 *  Handle showState
*/
void showStateCharCallback()
{ 
    BLE& ble = BLE::Instance(BLE::DEFAULT_INSTANCE);
    float VRMS = ((getVPP()/2.0) *0.707); 
    uint8_t AmpsRMS = (VRMS * 1000)/mVperAmp;
    printf("Value of AmpsRMS is %i mA ,mW: %i \n\r", AmpsRMS, (AmpsRMS*220)); 
    ble.gattServer().write(showStateChar.getValueHandle(), &AmpsRMS, sizeof(AmpsRMS));
}


/*
 * Initialization callback
 */
void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE &ble          = params->ble;
    ble_error_t error = params->error;
    
    if (error != BLE_ERROR_NONE) {
         printf("ERROR InitComplete\n\r");
        return;
    }

    ble.gap().onDisconnection(disconnectionCallback);
    ble.gattServer().onDataWritten(writeCharCallback);

    /* Setup advertising */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE); // BLE only, no classic BT
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED); // advertising type
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME)); // add name
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list)); // UUID's broadcast in advertising packet
    ble.gap().setAdvertisingInterval(100); // 100ms.

    /* Add our custom service */
    ble.addService(customService);

    /* Start advertising */
    ble.gap().startAdvertising();
}

/*
 *  Main loop
*/
int main(void)
{   
    /* initialize stuff */
    Ticker ticker;
    
    BLE& ble = BLE::Instance(BLE::DEFAULT_INSTANCE);
    ble.init(bleInitComplete);
 
    
    /* SpinWait for initialization to complete. This is necessary because the
     * BLE object is used in the main loop below. */
    while (ble.hasInitialized()  == false) { /* spin loop */ }
    
    ticker.attach(showStateCharCallback, 2);
    
    /* Infinite loop waiting for BLE interrupt events */
    while (true) {
        ble.waitForEvent(); /* Save power */
    }
}