MODSX::MODSX()
{
    for (int i = 0; i < 6; i++)
    {
        cellVolt[i] = 0.0f;
        lowestCellVolt[i] = 5.0f;
        highestCellVolt[i] = 0.0f;
        balanceState[i] = 0;
    }
    moduleVolt = 0.0f;
    temperatures[0] = 0.0f;
    temperatures[1] = 0.0f;
    lowestTemperature = 200.0f;
    highestTemperature = -100.0f;
    lowestModuleVolt = 200.0f;
    highestModuleVolt = 0.0f;
    exists = false;
    moduleAddress = 0;
    goodPackets = 0;
    badPackets = 0;
}

/*
Reading the status of the board to identify any flags, will be more useful when implementing a sleep cycle
*/
void MODSX::readStatus()
{
  uint8_t payload[3];
  uint8_t buff[8];
  payload[0] = moduleAddress << 1; //adresss
  payload[1] = REG_ALERT_STATUS;//Alert Status start
  payload[2] = 0x04;
  BMSUtil::sendDataWithReply(payload, 3, false, buff, 7);
  alerts = buff[3];
  faults = buff[4];
  COVFaults = buff[5];
  CUVFaults = buff[6];
}

uint8_t MODSX::getFaults()
{
    return faults;
}

uint8_t MODSX::getAlerts()
{
    return alerts;
}

uint8_t MODSX::getCOVCells()
{
    return COVFaults;
}

uint8_t MODSX::getCUVCells()
{
    return CUVFaults;
}

/*
Reading the setpoints, after a reset the default tesla setpoints are loaded
Default response : 0x10, 0x80, 0x31, 0x81, 0x08, 0x81, 0x66, 0xff
*/
/*
void MODSX::readSetpoint()
{
  uint8_t payload[3];
  uint8_t buff[12];
  payload[0] = moduleAddress << 1; //adresss
  payload[1] = 0x40;//Alert Status start
  payload[2] = 0x08;//two registers
  sendData(payload, 3, false);
  delay(2);
  getReply(buff);

  OVolt = 2.0+ (0.05* buff[5]);
  UVolt = 0.7 + (0.1* buff[7]);
  Tset = 35 + (5 * (buff[9] >> 4));
} */

bool MODSX::readModuleValues()
{
    uint8_t payload[4];
    uint8_t buff[50];
    uint8_t calcCRC;
    bool retVal = false;
    int retLen;
    float tempCalc;
    float tempTemp;

    payload[0] = moduleAddress << 1;

    readStatus();
    Logger::debug("Module %i   alerts=%X   faults=%X   COV=%X   CUV=%X", moduleAddress, alerts, faults, COVFaults, CUVFaults);

    payload[1] = REG_ADC_CTRL;
    payload[2] = 0b00111101; //ADC Auto mode, read every ADC input we can (Both Temps, Pack, 6 cells)
    BMSUtil::sendDataWithReply(payload, 3, true, buff, 3);

    payload[1] = REG_IO_CTRL;
    payload[2] = 0b00000011; //enable temperature measurement VSS pins
    BMSUtil::sendDataWithReply(payload, 3, true, buff, 3);

    payload[1] = REG_ADC_CONV; //start all ADC conversions
    payload[2] = 1;
    BMSUtil::sendDataWithReply(payload, 3, true, buff, 3);

    payload[1] = REG_GPAI; //start reading registers at the module voltage registers
    payload[2] = 0x12; //read 18 bytes (Each value takes 2 - ModuleV, CellV1-6, Temp1, Temp2)
    retLen = BMSUtil::sendDataWithReply(payload, 3, false, buff, 22);

    calcCRC = BMSUtil::genCRC(buff, retLen-1);
    Logger::debug("Sent CRC: %x     Calculated CRC: %x", buff[21], calcCRC);

    //18 data bytes, address, command, length, and CRC = 22 bytes returned
    //Also validate CRC to ensure we didn't get garbage data.
    if ( (retLen == 22) && (buff[21] == calcCRC) )
    {
        if (buff[0] == (moduleAddress << 1) && buff[1] == REG_GPAI && buff[2] == 0x12) //Also ensure this is actually the reply to our intended query
        {
            //payload is 2 bytes gpai, 2 bytes for each of 6 cell voltages, 2 bytes for each of two temperatures (18 bytes of data)
            moduleVolt = (buff[3] * 256 + buff[4]) * 0.002034609f;
            if (moduleVolt > highestModuleVolt) highestModuleVolt = moduleVolt;
            if (moduleVolt < lowestModuleVolt) lowestModuleVolt = moduleVolt;
            for (int i = 0; i < 6; i++)
            {
                cellVolt[i] = (buff[5 + (i * 2)] * 256 + buff[6 + (i * 2)]) * 0.000381493f;
                if (lowestCellVolt[i] > cellVolt[i]) lowestCellVolt[i] = cellVolt[i];
                if (highestCellVolt[i] < cellVolt[i]) highestCellVolt[i] = cellVolt[i];
            }

            //Now using steinhart/hart equation for temperatures. We'll see if it is better than old code.
            tempTemp = (1.78f / ((buff[17] * 256 + buff[18] + 2) / 33046.0f) - 3.57f);
            tempTemp *= 1000.0f;
            tempCalc =  1.0f / (0.0007610373573f + (0.0002728524832 * logf(tempTemp)) + (powf(logf(tempTemp), 3) * 0.0000001022822735f));

            temperatures[0] = tempCalc - 273.15f;

            tempTemp = 1.78f / ((buff[19] * 256 + buff[20] + 9) / 33068.0f) - 3.57f;
            tempTemp *= 1000.0f;
            tempCalc = 1.0f / (0.0007610373573f + (0.0002728524832 * logf(tempTemp)) + (powf(logf(tempTemp), 3) * 0.0000001022822735f));
            temperatures[1] = tempCalc - 273.15f;

            if (getLowTemp() < lowestTemperature) lowestTemperature = getLowTemp();
            if (getHighTemp() > highestTemperature) highestTemperature = getHighTemp();

            Logger::debug("Got voltage and temperature readings");
            goodPackets++;
            retVal = true;
        }
    }
    else
    {
        Logger::error("Invalid module response received for module %i  len: %i   crc: %i   calc: %i",
                      moduleAddress, retLen, buff[21], calcCRC);
        badPackets++;
    }

    Logger::debug("Good RX: %d       Bad RX: %d", goodPackets, badPackets);

     //turning the temperature wires off here seems to cause weird temperature glitches
   // payload[1] = REG_IO_CTRL;
   // payload[2] = 0b00000000; //turn off temperature measurement pins
   // BMSUtil::sendData(payload, 3, true);
   // delay(3);
   // BMSUtil::getReply(buff, 50);    //TODO: we're not validating the reply here. Perhaps check to see if a valid reply came back

    return retVal;
}

float MODSX::getCellVoltage(int cell)
{
    if (cell < 0 || cell > 5) return 0.0f;
    return cellVolt[cell];
}

float MODSX::getLowCellV()
{
    float lowVal = 10.0f;
    for (int i = 0; i < 6; i++) if (cellVolt[i] < lowVal) lowVal = cellVolt[i];
    return lowVal;
}

float MODSX::getHighCellV()
{
    float hiVal = 0.0f;
    for (int i = 0; i < 6; i++) if (cellVolt[i] > hiVal) hiVal = cellVolt[i];
    return hiVal;
}

float MODSX::getAverageV()
{
    float avgVal = 0.0f;
    for (int i = 0; i < 6; i++) avgVal += cellVolt[i];
    avgVal /= 6.0f;
    return avgVal;
}

float MODSX::getHighestModuleVolt()
{
    return highestModuleVolt;
}

float MODSX::getLowestModuleVolt()
{
    return lowestModuleVolt;
}

float MODSX::getHighestCellVolt(int cell)
{
    if (cell < 0 || cell > 5) return 0.0f;
    return highestCellVolt[cell];
}

float MODSX::getLowestCellVolt(int cell)
{
    if (cell < 0 || cell > 5) return 0.0f;
    return lowestCellVolt[cell];
}

float MODSX::getHighestTemp()
{
    return highestTemperature;
}

float MODSX::getLowestTemp()
{
    return lowestTemperature;
}

float MODSX::getLowTemp()
{
   return (temperatures[0] < temperatures[1]) ? temperatures[0] : temperatures[1];
}

float MODSX::getHighTemp()
{
   return (temperatures[0] < temperatures[1]) ? temperatures[1] : temperatures[0];
}

float MODSX::getAvgTemp()
{
    return (temperatures[0] + temperatures[1]) / 2.0f;
}

float MODSX::getModuleVoltage()
{
    return moduleVolt;
}

float MODSX::getTemperature(int temp)
{
    if (temp < 0 || temp > 1) return 0.0f;
    return temperatures[temp];
}

void MODSX::setAddress(int newAddr)
{
    if (newAddr < 0 || newAddr > MAX_MODULE_ADDR) return;
    moduleAddress = newAddr;
}

int MODSX::getAddress()
{
    return moduleAddress;
}

bool MODSX::isExisting()
{
    return exists;
}


void MODSX::setExists(bool ex)
{
    exists = ex;
}

void MODSX::balanceCells()
{
    uint8_t payload[4];
    uint8_t buff[30];
    uint8_t balance = 0;//bit 0 - 5 are to activate cell balancing 1-6

    payload[0] = moduleAddress << 1;
    payload[1] = REG_BAL_CTRL;
    payload[2] = 0; //writing zero to this register resets balance time and must be done before setting balance resistors again.
    BMSUtil::sendData(payload, 3, true);
    delay(2);
    BMSUtil::getReply(buff, 30);

    for (int i = 0; i < 6; i++)
    {
        if ( (balanceState[i] == 0) && (getCellVoltage(i) > settings.balanceVoltage) ) balanceState[i] = 1;

        if ( /*(balanceState[i] == 1) &&*/ (getCellVoltage(i) < (settings.balanceVoltage - settings.balanceHyst)) ) balanceState[i] = 0;

        if (balanceState[i] == 1) balance |= (1<<i);
    }

    if (balance != 0) //only send balance command when needed
    {
        payload[0] = moduleAddress << 1;
        payload[1] = REG_BAL_TIME;
        payload[2] = 0x82; //balance for two minutes if nobody says otherwise before then
        BMSUtil::sendData(payload, 3, true);
        delay(2);
        BMSUtil::getReply(buff, 30);

        payload[0] = moduleAddress << 1;
        payload[1] = REG_BAL_CTRL;
        payload[2] = balance; //write balance state to register
        BMSUtil::sendData(payload, 3, true);
        delay(2);
        BMSUtil::getReply(buff, 30);

        if (Logger::isDebug()) //read registers back out to check if everthing is good
        {
            Logger::debug("Reading back balancing registers:");
            delay(50);
            payload[0] = moduleAddress << 1;
            payload[1] = REG_BAL_TIME;
            payload[2] = 1; //expecting only 1 byte back
            BMSUtil::sendData(payload, 3, false);
            delay(2);
            BMSUtil::getReply(buff, 30);

            payload[0] = moduleAddress << 1;
            payload[1] = REG_BAL_CTRL;
            payload[2] = 1; //also only gets one byte
            BMSUtil::sendData(payload, 3, false);
            delay(2);
            BMSUtil::getReply(buff, 30);
        }
    }
}

uint8_t MODSX::getBalancingState(int cell)
{
    if (cell < 0 || cell > 5) return 0;
    return balanceState[cell];
}


/////////////////////////////////////////////From BMS Util.h////////////////////////////////////////////////////////////////////////////

    static uint8_t genCRC(uint8_t *input, int lenInput)
    {
        uint8_t generator = 0x07;
        uint8_t crc = 0;

        for (int x = 0; x < lenInput; x++)
        {
            crc ^= input[x]; /* XOR-in the next input byte */

            for (int i = 0; i < 8; i++)
            {
                if ((crc & 0x80) != 0)
                {
                    crc = (uint8_t)((crc << 1) ^ generator);
                }
                else
                {
                    crc <<= 1;
                }
            }
        }

        return crc;
    }

    static void sendData(uint8_t *data, uint8_t dataLen, bool isWrite)
    {
        uint8_t orig = data[0];
        uint8_t addrByte = data[0];
        if (isWrite) addrByte |= 1;
        SERIAL.write(addrByte);
        SERIAL.write(&data[1], dataLen - 1);  //assumes that there are at least 2 bytes sent every time. There should be, addr and cmd at the least.
        data[0] = addrByte;
        if (isWrite) SERIAL.write(genCRC(data, dataLen));

        if (Logger::isDebug())
        {
            SERIALCONSOLE.print("Sending: ");
            SERIALCONSOLE.print(addrByte, HEX);
            SERIALCONSOLE.print(" ");
            for (int x = 1; x < dataLen; x++) {
                SERIALCONSOLE.print(data[x], HEX);
                SERIALCONSOLE.print(" ");
            }
            if (isWrite) SERIALCONSOLE.print(genCRC(data, dataLen), HEX);
            SERIALCONSOLE.println();
        }

        data[0] = orig;
    }

    static int getReply(uint8_t *data, int maxLen)
    {
        int numBytes = 0;
        if (Logger::isDebug()) SERIALCONSOLE.print("Reply: ");
        while (SERIAL.available() && numBytes < maxLen)
        {
            data[numBytes] = SERIAL.read();
            if (Logger::isDebug()) {
                SERIALCONSOLE.print(data[numBytes], HEX);
                SERIALCONSOLE.print(" ");
            }
            numBytes++;
        }
        if (maxLen == numBytes)
        {
            while (SERIAL.available()) SERIAL.read();
        }
        if (Logger::isDebug()) SERIALCONSOLE.println();
        return numBytes;
    }

    //Uses above functions to send data then get the response. Will auto retry if response not
    //the expected return length. This helps to alleviate any comm issues. The Due cannot exactly
    //match the correct comm speed so sometimes there are data glitches.
    static int sendDataWithReply(uint8_t *data, uint8_t dataLen, bool isWrite, uint8_t *retData, int retLen)
    {
        int attempts = 1;
        int returnedLength;
        while (attempts < 4)
        {
            sendData(data, dataLen, isWrite);
            delay(2 * ((retLen / 8) + 1));
            returnedLength = getReply(retData, retLen);
            if (returnedLength == retLen) return returnedLength;
            attempts++;
        }
        return returnedLength; //failed to get a proper response.
    }



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
