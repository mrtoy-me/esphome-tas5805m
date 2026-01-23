// Set output channel volume using mixer gain lookup table
esp_err_t tas5805m_set_channel_gain(TAS5805M_EQ_CHANNELS channel, int8_t gain_db)
{
  ESP_LOGD(TAG, "%s: Setting channel %d volume to %d dB", __func__, channel, gain_db);

  if (gain_db < TAS5805M_MIXER_VALUE_MINDB || gain_db > TAS5805M_MIXER_VALUE_MAXDB) {
    ESP_LOGE(TAG, "%s: Invalid gain_db %d, must be between %d and %d", __func__, gain_db, TAS5805M_MIXER_VALUE_MINDB, TAS5805M_MIXER_VALUE_MAXDB);
    return ESP_ERR_INVALID_ARG;
  }

  /* Convert dB to linear gain and then to Q9.23 register format */
  float linear = powf(10.0f, ((float)gain_db) / 20.0f);
  uint32_t reg_value = tas5805m_float_to_q9_23(linear);

  uint8_t reg;
  if (channel == TAS5805M_EQ_CHANNELS_RIGHT) {
    reg = TAS5805M_REG_RIGHT_VOLUME;
  } else {
    // Default to left for any other value (matches other EQ channel helpers)
    reg = TAS5805M_REG_LEFT_VOLUME;
  }

  TAS5805M_SET_BOOK_AND_PAGE(TAS5805M_REG_BOOK_5, TAS5805M_REG_BOOK_5_VOLUME_PAGE);
  int ret = tas5805m_write_bytes(&reg, 1, (uint8_t *)&reg_value, sizeof(reg_value));
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "%s: Failed to write volume register 0x%02x: %s", __func__, reg, esp_err_to_name(ret));
  } else {
    ESP_LOGD(TAG, "%s: Wrote volume register 0x%02x with value 0x%08x", __func__, reg, reg_value);
  }

  TAS5805M_SET_BOOK_AND_PAGE(TAS5805M_REG_BOOK_CONTROL_PORT, TAS5805M_REG_PAGE_ZERO);
  if (ret == ESP_OK) {
    if (channel == TAS5805M_EQ_CHANNELS_RIGHT) {
      tas5805m_state.channel_gain_r = gain_db;
    } else {
      tas5805m_state.channel_gain_l = gain_db;
    }
  }

  return ret;


#define TAS5805M_MIXER_VALUE_MINDB -24
#define TAS5805M_MIXER_VALUE_MAXDB 24

#define TAS5805M_REG_BOOK_5_VOLUME_PAGE 0x2a
#define TAS5805M_REG_LEFT_VOLUME 0x24
#define TAS5805M_REG_RIGHT_VOLUME 0x28

typedef enum {
	TAS5805M_EQ_CHANNELS_LEFT = 0x00,
	TAS5805M_EQ_CHANNELS_RIGHT = 0x01,
	TAS5805M_EQ_CHANNELS_BOTH = 0x00,
} TAS5805M_EQ_CHANNELS;

uint32_t tas5805m_float_to_q9_23(float value)
{
    if (value > 255.999999f) value = 255.999999f;
    if (value < -256.0f)     value = -256.0f;

    int32_t fixed_val = (int32_t)(value * (1 << 23));
    uint32_t le_val = tas5805m_swap_endian_32((uint32_t)fixed_val);

    ESP_LOGD(TAG, "%s: value=%f -> fixed_val=%d, le_val=0x%08X",
             __func__, value, fixed_val, le_val);

    return le_val;
}

uint32_t tas5805m_swap_endian_32(uint32_t val)
{
    return ((val & 0xFF) << 24) |
           ((val & 0xFF00) << 8) |
           ((val & 0xFF0000) >> 8) |
           ((val >> 24) & 0xFF);
}