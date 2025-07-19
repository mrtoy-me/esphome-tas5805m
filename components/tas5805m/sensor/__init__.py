import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    STATE_CLASS_MEASUREMENT,
)

CONF_FAULTS_CLEARED = "faults_cleared"

from ..audio_dac import CONF_TAS5805M_ID, Tas5805mComponent, tas5805m_ns

FaultSensor = tas5805m_ns.class_("FaultSensor", cg.PollingComponent)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(FaultSensor),
            cv.GenerateID(CONF_TAS5805M_ID): cv.use_id(Tas5805mComponent),

            cv.Optional(CONF_FAULTS_CLEARED): sensor.sensor_schema(
                    accuracy_decimals=0,
                    state_class=STATE_CLASS_MEASUREMENT,
            ),
        }
    ).extend(cv.polling_component_schema("60s"))
)

async def to_code(config):
    tas5805m_component = await cg.get_variable(config[CONF_TAS5805M_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cg.register_parented(var, tas5805m_component)

    if clear_faults_config := config.get(CONF_FAULTS_CLEARED):
      sens = await sensor.new_sensor(clear_faults_config)
      cg.add(var.set_times_faults_cleared_sensor(sens))
