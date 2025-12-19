#ifdef CONFIG_64BIT
#define BITS_PER_LONG 64
#else
#define BITS_PER_LONG 32
#endif /* CONFIG_64BIT */

// source/include/linux/bist.h
#define BIT(nr)     (1UL << (nr))
#define BIT_ULL(nr) (1ULL << (nr))

#define GENMASK(h, l) \
    (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#define GENMASK_ULL(h, l) \
    (((~0ULL) - (1ULL << (l)) + 1) & (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))


// linux/source/include/linux/regmap.h#L1350

static const struct reg_field bq25890_reg_fields[] = {
    /* REG00 */
    [F_EN_HIZ] = REG_FIELD(0x00, 7, 7)
};

#define BQ25890_TSPCT_TBL_SIZE ARRAY_SIZE(bq25890_tspct_tbl)

// https://elixir.bootlin.com/linux/v6.12.6/source/include/linux/power_supply.h#L98
static const enum power_supply_property bq25890_power_supply_props[] = {
    POWER_SUPPLY_PROP_MANUFACTURER,
    POWER_SUPPLY_PROP_MODEL_NAME,
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_CHARGE_TYPE,
    POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_HEALTH,
    POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX,
    POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
    POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX,
    POWER_SUPPLY_PROP_PRECHARGE_CURRENT,
    POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT,
    POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_CURRENT_NOW,
    POWER_SUPPLY_PROP_TEMP,
};