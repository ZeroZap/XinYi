# openCH EXTI pending-state HIL

`xy_hal_exti_set_pending()` now sets the EXTI software pending source without also forcing the NVIC IRQ pending bit. This keeps `set_pending` distinct from `generate_software_interrupt`, which requests dispatch. Clearing pending before dispatch suppresses the callback, and unregistering the callback prevents later software-triggered callback delivery.

WCH-Link program, verify and reset passed. The smoke issued line-5 pending, cleared it before dispatch, verified no callback, then generated a software interrupt and verified one callback. After unregistering the callback, another software interrupt produced no additional callback.

This proves pending-state API separation and callback lifetime behavior. It does not prove a physical GPIO edge or external interrupt electrical path.
