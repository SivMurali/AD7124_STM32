/*
 * ad7124.c
 *
 *  Created on: Feb 6, 2025
 *      Author: user
 */

#include "ad7124.h"


Ad7124Register ad_reg[AD7124_REG_NO];

int begin(void) {
    AD7124_Reset();
    int ret = waitToPowerOn(1000);
    if (ret < 0) {
        return ret;
    }

    memset(ad_reg, 0, sizeof(ad_reg));
    for (int i = 0; i < AD7124_REG_NO; i++) {
        ad_reg[i].addr = (RegisterId)i;
    }
    
    // Set default sizes for AD7124 registers
    ad_reg[Status].size = 1;
    ad_reg[ADC_Control].size = 2;
    ad_reg[Data].size = 3;
    ad_reg[IOCon_1].size = 3;
    ad_reg[IOCon_2].size = 2;
    ad_reg[ID].size = 1;
    ad_reg[Error].size = 3;
    ad_reg[Error_En].size = 3;
    ad_reg[Mclk_Count].size = 1;
    for (int i = Channel_0; i <= Channel_15; i++) ad_reg[i].size = 2;
    for (int i = Config_0; i <= Config_7; i++) ad_reg[i].size = 2;
    for (int i = Filter_0; i <= Filter_7; i++) ad_reg[i].size = 3;
    for (int i = Offset_0; i <= Offset_7; i++) ad_reg[i].size = 3;
    for (int i = Gain_0; i <= Gain_7; i++) ad_reg[i].size = 3;

    return 0;
}

/**
 * @brf
 */
int status() {

  return (uint8_t)getRegister(Status, 1);
}

int currentChannel() {
  uint8_t ret = (uint8_t)status();

  if (ret < 0) {

    return ret;
  }
  return (uint8_t) (ret & AD7124_STATUS_REG_CH_ACTIVE (15));
}

int setChannel(uint8_t ch, uint8_t cfg, InputSel ainp, InputSel ainm, bool enable) {
    if ((ch < 16) && (cfg < 8)) {
        Ad7124Register *r = &ad_reg[ch];

        // Offset the channel index to match the register ID
        RegisterId regId = (RegisterId)(Channel_0 + ch);
        r->addr = regId;

        // Update the register value
        r->value = AD7124_CH_MAP_REG_SETUP(cfg) |
                   AD7124_CH_MAP_REG_AINP(ainp) |
                   AD7124_CH_MAP_REG_AINM(ainm) |
                   (enable ? AD7124_CH_MAP_REG_CH_ENABLE : 0);
        r->size = 2;
        // Write the updated register value
        return setRegister(r->addr, r->value, 2);
    }
    return -1; // Return error for invalid parameters
}

int enableChannel (uint8_t ch, bool enable)
{
	if (ch < 16)
	{
		RegisterId regId = (RegisterId)(Channel_0 + ch);
		long val = getRegister(regId, 2);
		if (val < 0) {
			return val;
		}

		if (enable) {
			val |= AD7124_CH_MAP_REG_CH_ENABLE;
		} else {
			val &= ~AD7124_CH_MAP_REG_CH_ENABLE;
		}

		return setRegister(regId, val, 2);
	}
	  return -1;
}

int channelConfig (uint8_t ch) {
	if (ch < 16) {
		long val = getRegister((RegisterId)(Channel_0 + ch), 2);
		if (val < 0) {
			return val;
		}
		return (val >> 12) & 0x07;
	}
	return -1;
}

int setConfig (uint8_t cfg, RefSel ref, PgaSel pga,
                       bool bipolar, BurnoutCurrent burnout)
{

  if (cfg < 8) {
    Ad7124Register * r = &ad_reg[cfg];

    cfg += Config_0;
    r->addr = (RegisterId)cfg;
    r->size = 2;

    r->value =    AD7124_CFG_REG_REF_SEL (ref) |
                  AD7124_CFG_REG_PGA (pga) |
                  (bipolar ? AD7124_CFG_REG_BIPOLAR : 0) |
                  AD7124_CFG_REG_BURNOUT (burnout) |
                  AD7124_CFG_REG_REF_BUFP | AD7124_CFG_REG_REF_BUFM |
                  AD7124_CFG_REG_AIN_BUFP | AD7124_CFG_REG_AINN_BUFM;
    return setRegister(r->addr, r->value, 2);
  }
  return -1;
}

int setConfigFilter (uint8_t cfg, FilterType filter, uint16_t fs, PostFilterType postfilter, bool rej60, bool single) {

  if (cfg < 8) {
    Ad7124Register * r = &ad_reg[cfg];

    cfg += Filter_0;
    r->addr = (RegisterId)cfg;
    r->size = 2;

    r->value = AD7124_FILT_REG_FILTER ( (uint32_t) filter) |
               AD7124_FILT_REG_POST_FILTER ( (uint32_t) postfilter) |
               AD7124_FILT_REG_FS (fs)    |
               (rej60 ? AD7124_FILT_REG_REJ60 : 0) |
               (single ? AD7124_FILT_REG_SINGLE_CYCLE : 0);
    return setRegister(r->addr, r->value, 3);
  }
  return -1;
}

int setConfigOffset (uint8_t cfg, uint32_t value) {

  if (cfg < 8) {

    cfg += Offset_0;

    return setRegister ( (RegisterId) cfg, value, 3);
  }
  return -1;
}

int setConfigGain (uint8_t cfg, uint32_t value) {

  if (cfg < 8) {

    cfg += Gain_0;
    return setRegister ( (RegisterId) cfg, value , 3);
  }
  return -1;
}

int setCurrentSource (uint8_t source, uint8_t ch, IoutCurrent current) {
  Ad7124Register * r = &ad_reg[IOCon_1];

  r->addr = IOCon_1;
  r->size = 3;
  
  // Read current value to preserve other bits
  long val = getRegister(IOCon_1, 3);
  if (val >= 0) {
      r->value = val;
  }

  if (source == 0) {
    r->value &= ~ (AD7124_IO_CTRL1_REG_IOUT0 (7) | AD7124_IO_CTRL1_REG_IOUT_CH0 (15));
    r->value |= AD7124_IO_CTRL1_REG_IOUT0 (current) | AD7124_IO_CTRL1_REG_IOUT_CH0 (ch);
  }
  else {
    r->value &= ~ (AD7124_IO_CTRL1_REG_IOUT1 (7) | AD7124_IO_CTRL1_REG_IOUT_CH1 (15));
    r->value |= AD7124_IO_CTRL1_REG_IOUT1 (current) | AD7124_IO_CTRL1_REG_IOUT_CH1 (ch);
  }
  return setRegister(r->addr, r->value, 3);
}

int setBiasPins (uint16_t pinMask) {
  Ad7124Register * r = &ad_reg[IOCon_2];

  r->addr = IOCon_2;
  r->size = 2;
  
  long val = getRegister(IOCon_2, 2);
  if (val >= 0) {
      r->value = val;
  }
  r->value &= 0xCC33;
  r->value |= pinMask;

  return setRegister(r->addr, r->value, 2);
}

int setAdcControl (OperatingMode mode,
                           PowerMode power_mode,
                           bool ref_en, ClkSel clk_sel) {
  Ad7124Register *r  = &ad_reg[ADC_Control];

  r->addr = ADC_Control;
  r->size = 2;
  r->value = AD7124_ADC_CTRL_REG_MODE (mode) |
             AD7124_ADC_CTRL_REG_POWER_MODE (power_mode) |
             AD7124_ADC_CTRL_REG_CLK_SEL (clk_sel) |
             (ref_en ? AD7124_ADC_CTRL_REG_REF_EN : 0) |
             AD7124_ADC_CTRL_REG_DOUT_RDY_DEL;

  return setRegister(r->addr, r->value, 2);
}

int setMode (OperatingMode mode) {
  Ad7124Register * r  = &ad_reg[ADC_Control];

  r->addr = ADC_Control;
  r->size = 2;
  
  long val = getRegister(ADC_Control, 2);
  if (val >= 0) {
      r->value = val;
  }
  
  r->value &= ~AD7124_ADC_CTRL_REG_MODE (0x0F); // clear mode
  r->value |= AD7124_ADC_CTRL_REG_MODE (mode);
  return setRegister(r->addr, r->value, 2);
}

int waitEndOfConversion (uint32_t timeout_ms) {

  return waitForConvReady (timeout_ms);
}

int startSingleConversion (uint8_t ch) {

  if (ch < 16) {
    return setMode (SingleConvMode);
  }
  return -1;
}
long read (uint8_t ch)
{
  int ret;
  
  // Enable the channel for conversion
  ret = enableChannel(ch, true);
  if (ret < 0) {
    return ret;
  }

  // Start single conversion
  ret = startSingleConversion(ch);
  if (ret < 0) {
    enableChannel(ch, false);
    return ret;
  }

  // Wait for the conversion to finish
  ret = waitEndOfConversion(250);
  if (ret < 0) {
    enableChannel(ch, false);
    return ret;
  }

  // Read the 24-bit conversion result from the Data register
  long sample = getRegister(Data, 3);

  // Disable the channel to conserve power and reset channel sequence
  enableChannel(ch, false);

  return sample;
}

 double toVoltage (long value, int gain, double vref, bool bipolar) {
  double voltage = (double) value;

  if (bipolar) {

    voltage = voltage / (double) 0x7FFFFFUL - 1;
  }
  else {

    voltage = voltage / (double) 0xFFFFFFUL;
  }

  voltage = voltage * vref / (double) gain;
  return voltage;
}

long getRegister (RegisterId id , uint8_t size) {

	int i = 0;
  Ad7124Register *r  = &ad_reg[id];
  uint8_t buffer[8] = {0};
  if ( (id >= Status) && (id < Reg_No))  {
    int ret;
   ret = AD7124_ReadRegister((RegisterId)id, buffer, size);
    if (ret < 0) {

      return ret;
    }
    r->value = 0;
    for (i = 0; i < size; i++)
    {
    	r->value <<= 8;
    	r->value += buffer[i];
    }

    return ad_reg[id].value;
  }
  return -1;
}

long setRegister (RegisterId id, long value,  uint8_t size) {

  Ad7124Register *r = &ad_reg[id];
  if ( (id >= Status) && (id < Reg_No))  {

	r->addr = (RegisterId)id;
	r->size = size;
    r->value = value;
    return AD7124_WriteRegister (r);
  }
  return -1;
}
int internalCalibration(uint8_t ch)
{
	  int ret;
	  uint8_t cfg;

	  	ret = setAdcControl(StandbyMode, FullPower, true, InternalClk);

	  if (ret < 0) {
	    return ret;
	  }

	  for (uint8_t c = 0; c < 16; c++) {

	    // disable all channels
	    ret = enableChannel(c, false);
	    if (ret < 0) {
	      return ret;
	    }
	  }

	  ret = channelConfig(ch);
	  if (ret < 0) {

	    return ret;
	  }
	  cfg = (uint8_t) ret;
	  ret = setConfigOffset(cfg, 0x800000);

	  ret = enableChannel(ch, true);
	  if (ret < 0) {
	    return ret;
	  }
	  ret = setAdcControl(InternalGainCalibrationMode, MidPower, true, InternalClk);			/* full scale */
	  ret = waitEndOfConversion(1000);
	  if (ret < 0) {
	    return ret;
	  }

	  ret = setAdcControl(InternalOffsetCalibrationMode, MidPower, true, InternalClk);			/* zero scale */

	  ret = waitEndOfConversion(1000);
	  if (ret < 0) {
	    return ret;
	  }

	  return enableChannel (ch, false);
}
void IntCalibration(IoutCurrent exciCurrent)
{
  setCurrentSource(0, 0, exciCurrent);
	HAL_Delay(1);
	internalCalibration(0);
	HAL_Delay(1);
	setCurrentSource(0, 0, CurrentOff);
	HAL_Delay(2);
	setCurrentSource(1, 1, exciCurrent);
	HAL_Delay(1);
	internalCalibration(1);
	HAL_Delay(1);
	setCurrentSource(1, 1, CurrentOff);
}
