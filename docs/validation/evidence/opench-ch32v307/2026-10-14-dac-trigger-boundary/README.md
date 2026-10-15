# openCH DAC trigger-boundary HIL

The WCH DAC HAL maps the supported timer trigger to the CH32V30x SDK's `DAC_Trigger_T6_TRGO`. Software trigger is rejected once timer-trigger mode is configured. The generic EXTI trigger enum is not implemented by this HAL because the SDK mapping would otherwise silently select EXTI9; initialization now rejects EXTI and unknown enum values explicitly instead of accepting an unintended trigger.

WCH-Link program, verify and reset passed. Runtime checked the DAC CTLR trigger field (`0x00040004`), verified timer-mode software-trigger rejection, and verified EXTI and unknown trigger rejection.

This proves trigger selection and API boundary behavior. It does not prove TIM6 configuration, TRGO frequency, external trigger timing, DAC analog response or oscilloscope-level output.
