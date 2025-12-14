import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.core import TimePeriod
from esphome.const import (
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

CONF_HAVE_FAULT =  "have_fault"
CONF_EXCLUDE = "exclude"
CONF_LEFT_CHANNEL_DC_FAULT = "left_channel_dc_fault"
CONF_RIGHT_CHANNEL_DC_FAULT = "right_channel_dc_fault"
CONF_LEFT_CHANNEL_OVER_CURRENT = "left_channel_over_current"
CONF_RIGHT_CHANNEL_OVER_CURRENT = "right_channel_over_current"
CONF_OTP_CRC_ERROR = "otp_crc_check"
CONF_BQ_WRITE_FAILED = "bq_write_failed"
CONF_CLOCK_FAULT = "clock fault"
CONF_PVDD_OVER_VOLTAGE = "pcdd_over_voltage"
CONF_PVDD_UNDER_VOLTAGE = "pcdd_under_voltage"
CONF_OVER_TEMP_SHUTDOWN = "over_temp_shutdown"
CONF_OVER_TEMP_WARNING = "over_temp_warning"

from .audio_dac import CONF_TAS5805M_ID, tas5805m_ns, Tas5805mComponent

ExcludeIgnoreModes = tas5805m_ns.enum("ExcludeIgnoreModes")
EXCLUDE_IGNORE_MODES = {
     "NONE"        : ExcludeIgnoreModes.NONE,
     "CLOCK_FAULT" : ExcludeIgnoreModes.CLOCK_FAULT,
}

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_TAS5805M_ID): cv.use_id(Tas5805mComponent),

    cv.Optional(CONF_HAVE_FAULT): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ).extend(
        {
            cv.Optional(CONF_EXCLUDE, default="CLOCK_FAULT"): cv.enum(
                        EXCLUDE_IGNORE_MODES, upper=True),
        }
    ),

    cv.Optional(CONF_LEFT_CHANNEL_DC_FAULT): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_RIGHT_CHANNEL_DC_FAULT): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_LEFT_CHANNEL_OVER_CURRENT): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_RIGHT_CHANNEL_OVER_CURRENT): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_OTP_CRC_ERROR): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_BQ_WRITE_FAILED): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_CLOCK_FAULT): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_PVDD_OVER_VOLTAGE): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_PVDD_UNDER_VOLTAGE): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_OVER_TEMP_SHUTDOWN): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_OVER_TEMP_WARNING): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
}

async def to_code(config):
    cg.add_define("USE_TAS5805M_BINARY_SENSOR")
    tas5805m_component = await cg.get_variable(config[CONF_TAS5805M_ID])

    if has_fault_config := config.get(CONF_HAVE_FAULT):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas5805m_component.set_have_fault_binary_sensor(sens))
        cg.add(tas5805m_component.config_exclude_fault(config[CONF_HAVE_FAULT][CONF_EXCLUDE]))

    if has_fault_config := config.get(CONF_LEFT_CHANNEL_DC_FAULT):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas5805m_component.set_left_channel_dc_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_RIGHT_CHANNEL_DC_FAULT):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas5805m_component.set_right_channel_dc_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_LEFT_CHANNEL_OVER_CURRENT):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas5805m_component.set_left_channel_over_current_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_RIGHT_CHANNEL_OVER_CURRENT):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas5805m_component.set_right_channel_over_current_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_OTP_CRC_ERROR):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas5805m_component.set_otp_crc_check_error_binary_sensor(sens))

    if has_fault_config := config.get(CONF_BQ_WRITE_FAILED):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas5805m_component.set_bq_write_failed_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_CLOCK_FAULT):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas5805m_component.set_clock_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_PVDD_OVER_VOLTAGE):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas5805m_component.set_pvdd_over_voltage_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_PVDD_UNDER_VOLTAGE):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas5805m_component.set_pvdd_under_voltage_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_OVER_TEMP_SHUTDOWN):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas5805m_component.set_over_temperature_shutdown_fault_binary_sensor(sens))

    if has_fault_config := config.get(CONF_OVER_TEMP_WARNING):
        sens = await binary_sensor.new_binary_sensor(has_fault_config)
        cg.add(tas5805m_component.set_over_temperature_warning_binary_sensor(sens))

