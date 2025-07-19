# ESPHome component for ESP32-S3 Louder and ESP32 Louder
The ESP32-S3 Louder and ESP32 Louder uses a TAS5805M DAC. This ESPHome external component<BR>
is based on the following ESP32 Platform TAS5805M DAC driver:<BR>
https://github.com/sonocotta/esp32-tas5805m-dac/tree/main by Andriy Malyshenko<BR>
which is licenced under GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007.<BR>
Information from this repository has also been used/reproduced in this read.me to provide a better<BR>
understanding of how to use this component to generate firmware using Esphome Builder.<BR>

# Usage: Tas5805m component on Github
This component requires Esphome version 2025.7.0 or later.

The following yaml can be used so ESPHome accesses the component files:
```
external_components:
  - source: github://mrtoy-me/esphome-tas5805m@main
    components: [ tas5805m ]
    refresh: 0s
```

# Overview
This component is specifically intended for use with ESP32-S3 Louder or ESP32 Louder<BR>
which both have a TAS5805M DAC for producing audio.<BR>
This component uses the Esphome ESP_IDF framwork and must be configured accordingly.<BR>
While this component works with both versions of the "Louder", the ESP32-S3 Louder<BR>
has larger PSRAM and hence has better performance than the ESP32 Louder.<BR>

The component allows many features of the TAS5808M DAC to be configured<BR>
prior to firmware generation or configured/controlled through Homeassistant.<BR>
Fault sensors can also be configured, so their status is visible in Homeassistant.<BR>

The component YAML configuration uses the esphome Audio DAC Core component,<BR>
and is configured under **audio_dac:** as **- platform: tas5805m**<BR>

The TAS5805M DAC is controlled by I2C so the component requires configuration of<BR>
**i2c:** with **sda:** and **scl:** pins.<BR>

Appropriate configuration of psram, i2s_audio, speaker and mediaplayer are required.<BR>
Example YAML configurations are provided and are listed at the end of this read.me<BR>

## Component Features
The component communicates with ESP32 Louder's TAS5805M and provides the following features:<BR>
- initialise TAS5805M DAC
- enable/disable TAS5805M DAC
- adjust TAS5805M maximum and minimum volume level
- set DAC mode
- set Mixer mode
- set EQ Control state and EQ gains
- get fault states
- automatically clear fault states
- set Analog Gain
- set Volume
- set Mute state

YAML configuration includes:
- two optional tas5805m platform Switch configurations - Enable Louder and Enable EQ Control
- 15 optional EQ Gain tas5805m platform Numbers (all required or none configured) to control EQ gains
- 12 optional tas5805m platform Binary Sensors corresonding to TAS5805M fault codes (all optional)
- an optional tas5805m platform Sensor providing the number of times a fault was detected and fault cleared

# Louder TAS5805M Features
## Analog Gain and Digital Volume
The analog gain setting ensures that the output signal is not clipped at different supply voltage levels.<BR>
With TAS5805M analog gain set at the appropriate level, the TAS5805M digital volume is used<BR>
to set the audio volume. Keep in mind, it is perfectly safe to set the analog gain at a lower level.<BR>
Note: that the component allows defining analog gain in YAML and cannot be altered at runtime.<BR>

## DAC Mode
TAS5805M has a bridge mode of operation, that causes both output drivers to synchronize and push out<BR>
the same audio with double the power. Typical setup for each of the Dac Modes is shown in the following table.<BR>
Note: that the component allows defining Dac Mode in YAML and cannot be altered at runtime.<BR>

|   | BTL (default, STEREO) | PBTL (MONO, rougly double power) |
|---|-----------------------|---------------------------|
| Descriotion | Bridge Tied Load, Stereo | Parallel Bridge Tied Load, Mono |
| Rated Power | 2×23W (8-Ω, 21 V, THD+N=1%) | 45W (4-Ω, 21 V, THD+N=1%) |
| Schematics | ![image](https://github.com/sonocotta/esp32-audio-dock/assets/5459747/e7ada8c0-c906-4c08-ae99-be9dfe907574) | ![image](https://github.com/sonocotta/esp32-audio-dock/assets/5459747/55f5315a-03eb-47c8-9aea-51e3eb3757fe)
| Speaker Connection | ![image](https://github.com/user-attachments/assets/8e5e9c38-2696-419b-9c5b-d278c655b0db) | ![image](https://github.com/user-attachments/assets/8aba6273-84c4-45a8-9808-93317d794a44)


## Mixer Mode
Mixer mode allows mixing of channel signals and route them to the appropriate audio channel.<BR>
The typical setup for the mixer is to send Left channel audio to the Left driver, and Right channel to the Right.<BR>
A common alternative is to combine both channels into true Mono (you need to reduce both to -3Db to compensate for signal doubling).<BR>
In BTL Dac Mode, the mixer mode can be set to STEREO, INVERSE_STEREO, MONO, LEFT or RIGHT while<BR>
in PBTL Dac Mode, the mixer mode can be set to MONO, LEFT or RIGHT.<BR>


## EQ Band Gains
TAS5805M has a powerful 15-channel EQ that allows defining each channel's transfer function using BQ coefficients.<BR>
For practical purposes, the audio range is split into 15 bands, defining for each a -15 to +15 dB gain adjustment range<BR>
and appropriate bandwidth to cause mild overlap. This keeps the curve flat enough to not cause distortions<BR>
even in extreme settings, but also allows a wide range of transfer characteristics.<BR>

| Band | Center Frequency (Hz) | Frequency Range (Hz) | Q-Factor (Approx.) |
|------|-----------------------|----------------------|--------------------|
| 1    | 20                    | 10–30                | 2                  |
| 2    | 31.5                  | 20–45                | 2                  |
| 3    | 50                    | 35–70                | 1.5                |
| 4    | 80                    | 55–110               | 1.5                |
| 5    | 125                   | 85–175               | 1                  |
| 6    | 200                   | 140–280              | 1                  |
| 7    | 315                   | 220–440              | 0.9                |
| 8    | 500                   | 350–700              | 0.9                |
| 9    | 800                   | 560–1120             | 0.8                |
| 10   | 1250                  | 875–1750             | 0.8                |
| 11   | 2000                  | 1400–2800            | 0.7                |
| 12   | 3150                  | 2200–4400            | 0.7                |
| 13   | 5000                  | 3500–7000            | 0.6                |
| 14   | 8000                  | 5600–11200           | 0.6                |


## Fault States
TAS5805M has a fault detection system that allows it to self-diagnose issues with power, data signal, short circuits, overheating etc.<BR>
The general pattern for fault detection is periodic check of fault registers, and when there are any faults, provide notification<BR>
through sensor/s and clear any fault afterwards.<BR>
<BR>

# Activation of Mixer mode and EQ Gains
For software configuration of the Mixer and EQ Gains, the Louder's TAS5805M must have received a stable I2S signal.<BR>
If a Mixer setting (other than default) or EQ Band Gain Numbers are configured, what this means for this component is that<BR>
before the component writes these settings to the TAS5805M, the TAS5805M must have received some audio.<BR>

## Typical Use Case - Speaker Mediaplayer
The typical way of handling this requirement is where speaker mediaplayer component is configured to play audio during boot.<BR>
In this case, a short sound file is configured under **mediaplayer:** and configuration added under **esphome:**<BR>
to play that short sound at the correct point in the boot process.<BR>

Configuration required to be included under **mediaplayer:** YAML is:
```
files:
    id: startup_sync_sound
    file: https://github.com/mrtoy-me/esphome-tas5805m/raw/main/components/tas5805m/boot_louder.flac
```
Configuration required to be included under **esphome:** YAML:
```
on_boot:
    priority: 220.0
    then:
      media_player.speaker.play_on_device_media_file: startup_sync_sound
```
The **audio_dac:** has an optional configuration variable called **refresh_eq:**<BR>
The default configuration of **refresh_eq: BY_GAIN** matches the above use case and<BR>
therefore is not required(can be omitted) from the **audio_dac:** YAML configuration.<BR>

## Use Case where Speaker Mediaplayer is not used (eg using a SnapCast client component)
Another use case, is use of Snapcast client component instead of Speaker Mediaplayer component to produce the required audio.<BR>
In this use case, the following "workaround" is necessary to play audio through the Louder's TAS58065M before<BR>
the component writes the Mixer and EQ Gain settings to the TAS5805M. This workaround requires the user<BR>
to start playing audio then turn on the Enable EQ Switch. The following changed configuration is required:<BR>

1) Configure **audio_dac:** with optional configuration variable and value **refresh_eq:  BY_GAIN**
2) Configure **switch: - platform: tas5805m** with **enable_eq:** as follows:
```
switch:
  - platform: tas5805m
    enable_eq:
      name: Enable EQ Control
      restore_mode: ALWAYS_OFF
```

3) After Louder has booted, manually initiate playing of some audio
4) Turn Enable EQ Switch On


# YAML configuration

## Audio Dac

Example typical configuration:
```
audio_dac:
  - platform: tas5805m
    id: tas5805_dac
    enable_pin: GPIOxx
    analog_gain: -9db
    dac_mode: BTL
    mixer_mode: STEREO
    volume_max: 0dB
    volume_min: -60db
    update_interval: 10s
```
Configuration variables:
- **enable_pin:** (*Required*): GPIOxx, enable pin of ESP32 Louder<BR>

- **analog_gain:** (*Optional*): dB values from -15.5dB to 0dB in 0.5dB increments<BR>
  Defaults to -15.5dbB. A setting of -15.5db is typical when 5v is used to power the Louder<BR>

- **dac_mode:** (*Optional*): valid values of BTL or PBTL. Defaults to BTL<BR>

- **mixer_mode:** (*Optional*): values of STEREO, INVERSE_STEREO, MONO, LEFT or RIGHT<BR>
  Defaults to STEREO. Note: for PBTL Dac Mode, only MONO, LEFT or RIGHT are valid.<BR>

- **volume_max:** (*Optional*): whole dB values from -103dB to 24dB. Defaults to 24dB<BR>

- **volume_min:** (*Optional*): whole dB values from -103dB to 24dB. Defaults to -103dB<BR>

- **update_interval:** (*Optional*): defines the interval (seconds) at which faults will be checked<BR>
  and then if detected will be cleared at next interval. Defaults to 30s.<BR>

- **refresh_eq:** (*Optional*): valid values BY_GAIN or BY_SWITCH. Defaults to BY_GAIN<BR>
  This setting can normally be ignored and omitted if you are using Speaker Mediaplayer component and<BR>
  is intended for use when Snapcast client component is used instead of Speaker Mediaplayer.<BR>
  When a Snapcast client component is configured, the BY_SWITCH value should be used.<BR>
  See information under "Activation of Mixer mode and EQ Gains" section above and the provided YAML examples.<BR>


## Switches
Two tas5805m platform switches can be configured to provide switches in Homeassistant.<BR>
- Enable Louder Switch, more specifically places TAS5805M into Play mode or into low power Sleep mode<BR>
- Enable EQ Control Switch which switches the TAS5805M DAC EQ control On/Off. This switch works<BR>
  in conjunction with configuration of tas5805m platform numbers which configure gain of EQ Bands.<BR>

The example YAML also includes an **interval:** and **mediaplayer:** configuration to trigger<BR>
Enable Louder Switch Off when there is no music player activity (idle or paused) for the defined time and<BR>
when music player activity is detected (by mediaplayer), the Enable Louder Switch is triggered On .<BR>
The example interval configuration requires some configuration of **mediaplayer:**<BR>
which is also shown in the YAML examples.<BR>

Configuration of tas5805m switches in typical use case:<BR>

```switch:
  - platform: tas5805m
    enable_dac:
      name: Enable Louder
      id: enable_louder
      restore_mode: ALWAYS_ON
    enable_eq:
      name: Enable EQ Control
      restore_mode: RESTORE_DEFAULT_ON
```
Configuration headers:
- **enable_dac:** (*Optional*): allows the definition of a switch to enable/disable the TAS5805M DAC.<BR>
Switch On (enabled) places TAS5805M into Play mode while Switch Off (disabled)<BR>
places TAS5805M into low power Sleep mode.<BR>
    Configuration variables:
    - **restore_mode:** (optional but recommended): **RESTORE_DEFAULT_ON** is recommended.<BR>

- **enable_eq:** (*Optional*): allows the definition of a switch to turn on/off the TAS5805M DAC<BR>
EQ Control Mode. Switch On enables TAS5805M EQ Control while Switch Off disables TAS5805M EQ Control.<BR>
    Configuration variables:
    - **restore_mode:** (optional but recommended): **RESTORE_DEFAULT_ON** is recommended<BR>
      For typical use case where speaker mediaplayer is use for audio.<BR>
      For use case, where SnapCast client component is used instead of Speaker mediaplayer component,<BR>
      **ALWAYS_OFF** is recommended.<BR>

## EQ Band Gain Numbers
15 EQ Band Gain Numbers can be configured for controlling the gain of each EQ Band in Homeassistant.<BR>
The number configuration heading for each number is shown below with an example name configured.<BR>
Defining **number: -platform: tas5805m** requires all 15 EQ Gain Band headings to be configured.<BR>
For TAS5805M EQ Band Gains to configure correctly requires some addition YAML configuration, refer<BR>
to the "Activation of Mixer mode and EQ Gains" section above and the provided YAML examples.<BR>

```
number:
  - platform: tas5805m
      eq_gain_band20Hz:
        name: Gain ---20Hz
      eq_gain_band31.5Hz:
        name: Gain ---31.5Hz
      eq_gain_band50Hz:
        name: Gain ---50Hz
      eq_gain_band80Hz:
        name: Gain ---80Hz
      eq_gain_band125Hz:
        name: Gain --125Hz
      eq_gain_band200Hz:
        name: Gain --200Hz
      eq_gain_band315Hz:
        name: Gain --315Hz
      eq_gain_band500Hz:
        name: Gain --500Hz
      eq_gain_band800Hz:
        name: Gain --800Hz
      eq_gain_band1250Hz:
        name: Gain -1250Hz
      eq_gain_band2000Hz:
        name: Gain -2000Hz
      eq_gain_band3150Hz:
        name: Gain -3150Hz
      eq_gain_band5000Hz:
        name: Gain -5000Hz
      eq_gain_band8000Hz:
        name: Gain -8000Hz
      eq_gain_band16000Hz:
        name: Gain 16000Hz
```

## Announce Volume Template Number
The example YAML defines an Announce Volume template number which can be used in conjuction<BR>
with the **mediaplayer:** YAML configurations for adjusting the announcement pipeline audio<BR>
volume separately to the media pipeline volume. This is useful for a Text-to-Speech announcements<BR>
that may have a different volume level to the audio playing through the media pipeline.<BR>
The YAML examples provide an example of how this can be configured.<BR>

## Binary Sensors
Binary sensors can be configured which correspond to fault codes from the TAS5805M.<BR>
The tas5805m binary sensor platform has configuration headings for each binary sensor<BR>
as shown below with an example name configured.<BR>
All 12 binary sensors can be optionally defined but it is recommended that at minimum,<BR>
one binary sensor **have_fault:** is configured. The **have_fault:** binary sensor<BR>
activates if any the TAS5805M faults conditions activate.<BR>
Note: binary sensors are updated at the **update interval:** defined under **audio_dac:** or<BR>
if not defined default to 30s.<BR>

```
binary_sensor:
  - platform: tas5805m
    have_fault:
      name: Any Faults
    left_channel_dc_fault:
      name: Left Channel DC Fault
    right_channel_dc_fault:
      name: Right Channel DC Fault
    left_channel_over_current:
      name: Left Channel Over Current
    right_channel_over_current:
      name: Right Channel Over Current
    otp_crc_check:
      name: CRC Check Fault
    bq_write_failed:
      name: BQ Write Failure
    clock fault:
      name: I2S Clock Fault
    pcdd_over_voltage:
      name: PCDD Over Voltage
    pcdd_under_voltage:
      name: PCDD Under Voltage
    over_temp_shutdown:
      name: Over Temperature Shutdown Fault
    over_temp_warning:
      name: Over Temperature Warning
```

## Sensor
Under the tas5805m sensor platform, one sensor under configuration heading **faults_cleared:**<BR>
can be optionally configured. This sensor counts the number of time a fault was detected<BR>
and then subsequently cleared by the component.<BR>
```
sensor:
  - platform: tas5805m
    faults_cleared:
      name: "Times Faults Cleared"
```
Configuration variables:
- **update interval:** (*Optional*): The interval at which the sensor is updated. Defaults to 60s<BR>


# YAML examples in this Repository
The following example YAML configurations are provided under the **Example YAML** directory.<BR>
Note that these example configurations are expressly provided under the **Disclaimer of Warranty**<BR>
and **Limititation of Liability** conditions of GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007.<BR>

- ESP32-S3 Louder with Speaker Mediaplayer handling audio in **esp32S3_louder_idf_media.yaml**<BR>
- ESP32 Louder with Speaker Mediaplayer handling audio in **esp32_louder_idf_media.yaml**<BR><BR>

- ESP32-S3 Louder with esphome-snapclient https://github.com/c-MM/esphome-snapclient
  handling audio in **esp32S3_snapclient_idf_media.yaml**<BR>
- ESP32-S3 Louder with esphome-snapclient https://github.com/c-MM/esphome-snapclient
  handling audio in **esp32S3_snapclient_idf_media.yaml**<BR>
