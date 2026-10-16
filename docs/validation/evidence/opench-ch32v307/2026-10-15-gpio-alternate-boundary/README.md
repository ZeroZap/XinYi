# openCH GPIO alternate boundary HIL

CH32V30x selects most peripheral routing through fixed pin mappings and AFIO remap controls, not STM32-style per-pin alternate-function numbers. The WCH GPIO HAL now accepts only alternate value zero instead of caching unsupported nonzero values as if hardware had applied them.

WCH-Link program, verify and reset passed. PC4 initialization with alternate 1 was rejected without changing CFGLR or claiming ownership. Updating an initialized PC4 to alternate 1 was also rejected without changing CFGLR or cached alternate state; alternate 0 remained accepted.

This validates the API boundary and failure atomicity. It does not configure or validate AFIO peripheral remapping.
