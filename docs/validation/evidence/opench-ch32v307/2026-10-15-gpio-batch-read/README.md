# openCH GPIO batch-read HIL

WCH batch reads now reject an empty mask and validate that every selected pin has been initialized before reading INDR. This prevents an apparently valid value from silently including unowned pins.

WCH-Link program, verify and reset passed. PC4/PC5 reads were rejected while either pin was uninitialized, accepted once both were configured as inputs, and rejected again after PC5 was deinitialized. Returned bits were confined to the requested mask.

This proves HAL ownership validation and input-register masking. It does not prove externally driven voltage levels or transitions.
