/**
 * @file lm3s6965.h
 * @brief TI Stellaris LM3S6965 寄存器定义 (QEMU 模拟)
 * 
 * QEMU lm3s6965evb 开发板寄存器映射
 * 参考：QEMU hw/arm/stellaris.c
 */

#ifndef __LM3S6965_H__
#define __LM3S6965_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 *  内存映射
 *===========================================================================*/

#define FLASH_BASE          0x00000000U
#define FLASH_SIZE          (256 * 1024U)

#define SRAM_BASE           0x20000000U
#define SRAM_SIZE           (64 * 1024U)

#define PERIPH_BASE         0x40000000U

/*============================================================================
 *  系统控制模块 (SYSCTL)
 *===========================================================================*/

#define SYSCTL_BASE         (PERIPH_BASE + 0x0001F000U)

typedef struct {
    volatile uint32_t DID0;         /* 0x000 - Device Identification 0 */
    volatile uint32_t DID1;         /* 0x004 - Device Identification 1 */
    volatile uint32_t DID2;         /* 0x008 - Device Identification 2 */
    volatile uint32_t DC0;          /* 0x00C - Device Capabilities 0 */
    volatile uint32_t DC1;          /* 0x010 - Device Capabilities 1 */
    volatile uint32_t DC2;          /* 0x014 - Device Capabilities 2 */
    volatile uint32_t DC3;          /* 0x018 - Device Capabilities 3 */
    volatile uint32_t DC4;          /* 0x01C - Device Capabilities 4 */
    volatile uint32_t RESERVED1[3];
    volatile uint32_t PBORCTL;      /* 0x030 - Brown-Out Reset Control */
    volatile uint32_t LDOPCTL;      /* 0x034 - LDO Power Control */
    volatile uint32_t SRCR0;        /* 0x038 - Software Reset Control 0 */
    volatile uint32_t SRCR1;        /* 0x03C - Software Reset Control 1 */
    volatile uint32_t SRCR2;        /* 0x040 - Software Reset Control 2 */
    volatile uint32_t RIS;          /* 0x044 - Raw Interrupt Status */
    volatile uint32_t IMC;          /* 0x048 - Interrupt Mask Control */
    volatile uint32_t MISC;         /* 0x04C - Masked Interrupt Status and Clear */
    volatile uint32_t RESC;         /* 0x050 - Reset Cause */
    volatile uint32_t RCC;          /* 0x054 - Run-Mode Clock Configuration */
    volatile uint32_t PLLCFG;       /* 0x058 - PLL Configuration */
    volatile uint32_t GCC;          /* 0x05C - GPIO Clock Configuration */
    volatile uint32_t SPLITR;       /* 0x060 - Memory Split Control */
    volatile uint32_t USECRL;       /* 0x064 - Microsecond Reload Value */
    volatile uint32_t GPIOHS;       /* 0x068 - GPIO High Speed */
    volatile uint32_t RCC2;         /* 0x070 - Run-Mode Clock Configuration 2 */
    volatile uint32_t MOSCCTL;      /* 0x07C - Main Oscillator Control */
} lm3s_sysctl_t;

#define LM3S_SYSCTL   ((lm3s_sysctl_t *)SYSCTL_BASE)

/*============================================================================
 *  GPIO 端口
 *===========================================================================*/

#define GPIO_PORTA_BASE   (PERIPH_BASE + 0x00020000U)
#define GPIO_PORTB_BASE   (PERIPH_BASE + 0x00021000U)
#define GPIO_PORTC_BASE   (PERIPH_BASE + 0x00022000U)
#define GPIO_PORTD_BASE   (PERIPH_BASE + 0x00023000U)
#define GPIO_PORTE_BASE   (PERIPH_BASE + 0x00024000U)
#define GPIO_PORTF_BASE   (PERIPH_BASE + 0x00025000U)
#define GPIO_PORTG_BASE   (PERIPH_BASE + 0x00026000U)
#define GPIO_PORTH_BASE   (PERIPH_BASE + 0x00027000U)

typedef struct {
    volatile uint32_t DATA[256];    /* 0x000-0x3FC - Data (offset selects mask) */
    volatile uint32_t DIR;          /* 0x400 - Direction */
    volatile uint32_t IS;           /* 0x404 - Interrupt Sense */
    volatile uint32_t IBE;          /* 0x408 - Interrupt Both Edges */
    volatile uint32_t IEV;          /* 0x40C - Interrupt Event */
    volatile uint32_t IM;           /* 0x410 - Interrupt Mask */
    volatile uint32_t RIS;          /* 0x414 - Raw Interrupt Status */
    volatile uint32_t MIS;          /* 0x418 - Masked Interrupt Status */
    volatile uint32_t ICR;          /* 0x41C - Interrupt Clear */
    volatile uint32_t AFSEL;        /* 0x420 - Alternate Function Select */
    volatile uint32_t RESERVED1[221];
    volatile uint32_t DR2R;         /* 0x700 - 2mA Drive Select */
    volatile uint32_t DR4R;         /* 0x704 - 4mA Drive Select */
    volatile uint32_t DR8R;         /* 0x708 - 8mA Drive Select */
    volatile uint32_t ODR;          /* 0x70C - Open Drain Select */
    volatile uint32_t PUR;          /* 0x710 - Pull-Up Select */
    volatile uint32_t PDR;          /* 0x714 - Pull-Down Select */
    volatile uint32_t SLR;          /* 0x718 - Slew Rate Control */
    volatile uint32_t DEN;          /* 0x71C - Digital Enable */
    volatile uint32_t LOCK;         /* 0x720 - Lock */
    volatile uint32_t CR;           /* 0x724 - Commit */
    volatile uint32_t AMSEL;        /* 0x728 - Analog Mode Select */
    volatile uint32_t PCTL;         /* 0x72C - Port Control */
    volatile uint32_t ADCCTL;       /* 0x730 - ADC Control */
    volatile uint32_t DMACTL;       /* 0x734 - DMA Control */
} lm3s_gpio_t;

#define LM3S_GPIOA    ((lm3s_gpio_t *)GPIO_PORTA_BASE)
#define LM3S_GPIOB    ((lm3s_gpio_t *)GPIO_PORTB_BASE)
#define LM3S_GPIOC    ((lm3s_gpio_t *)GPIO_PORTC_BASE)
#define LM3S_GPIOD    ((lm3s_gpio_t *)GPIO_PORTD_BASE)
#define LM3S_GPIOE    ((lm3s_gpio_t *)GPIO_PORTE_BASE)
#define LM3S_GPIOF    ((lm3s_gpio_t *)GPIO_PORTF_BASE)
#define LM3S_GPIOG    ((lm3s_gpio_t *)GPIO_PORTG_BASE)
#define LM3S_GPIOH    ((lm3s_gpio_t *)GPIO_PORTH_BASE)

/* GPIO 引脚定义 */
#define GPIO_PIN_0      0x01U
#define GPIO_PIN_1      0x02U
#define GPIO_PIN_2      0x04U
#define GPIO_PIN_3      0x08U
#define GPIO_PIN_4      0x10U
#define GPIO_PIN_5      0x20U
#define GPIO_PIN_6      0x40U
#define GPIO_PIN_7      0x80U

/*============================================================================
 *  UART 模块
 *===========================================================================*/

#define UART0_BASE        (PERIPH_BASE + 0x000C0000U)
#define UART1_BASE        (PERIPH_BASE + 0x000C1000U)
#define UART2_BASE        (PERIPH_BASE + 0x000C2000U)

typedef struct {
    volatile uint32_t DR;           /* 0x000 - Data */
    volatile uint32_t ECR;          /* 0x004 - Error Clear */
    volatile uint32_t RESERVED1[6];
    volatile uint32_t FR;           /* 0x018 - Flag */
    volatile uint32_t RESERVED2;
    volatile uint32_t ILPR;         /* 0x020 - IrDA Low-Power Counter */
    volatile uint32_t IBRD;         /* 0x024 - Integer Baud Rate Divisor */
    volatile uint32_t FBRD;         /* 0x028 - Fractional Baud Rate Divisor */
    volatile uint32_t LCRH;         /* 0x02C - Line Control */
    volatile uint32_t CTL;          /* 0x030 - Control */
    volatile uint32_t IFLS;         /* 0x034 - Interrupt FIFO Level Select */
    volatile uint32_t IM;           /* 0x038 - Interrupt Mask */
    volatile uint32_t RIS;          /* 0x03C - Raw Interrupt Status */
    volatile uint32_t MIS;          /* 0x040 - Masked Interrupt Status */
    volatile uint32_t ICR;          /* 0x044 - Interrupt Clear */
    volatile uint32_t DMACTL;       /* 0x048 - DMA Control */
} lm3s_uart_t;

#define LM3S_UART0    ((lm3s_uart_t *)UART0_BASE)
#define LM3S_UART1    ((lm3s_uart_t *)UART1_BASE)
#define LM3S_UART2    ((lm3s_uart_t *)UART2_BASE)

/* UART 标志位 */
#define UART_FR_RXFE    0x10U       /* Receive FIFO Empty */
#define UART_FR_TXFF    0x20U       /* Transmit FIFO Full */

/* UART 线控制 */
#define UART_LCRH_WLEN_8  0x60U     /* 8 data bits */
#define UART_LCRH_FEN     0x10U     /* FIFO Enable */

/* UART 控制 */
#define UART_CTL_UARTEN   0x01U     /* UART Enable */
#define UART_CTL_TXE      0x100U    /* Transmit Enable */
#define UART_CTL_RXE      0x200U    /* Receive Enable */

/*============================================================================
 *  SSI (SPI) 模块
 *===========================================================================*/

#define SSI0_BASE         (PERIPH_BASE + 0x00008000U)
#define SSI1_BASE         (PERIPH_BASE + 0x00009000U)

typedef struct {
    volatile uint32_t CR0;          /* 0x000 - Control 0 */
    volatile uint32_t CR1;          /* 0x004 - Control 1 */
    volatile uint32_t DR;           /* 0x008 - Data */
    volatile uint32_t SR;           /* 0x00C - Status */
    volatile uint32_t CPSR;         /* 0x010 - Clock Prescale */
    volatile uint32_t IM;           /* 0x014 - Interrupt Mask */
    volatile uint32_t RIS;          /* 0x018 - Raw Interrupt Status */
    volatile uint32_t MIS;          /* 0x01C - Masked Interrupt Status */
    volatile uint32_t ICR;          /* 0x020 - Interrupt Clear */
    volatile uint32_t DMACTL;       /* 0x024 - DMA Control */
} lm3s_ssi_t;

#define LM3S_SSI0     ((lm3s_ssi_t *)SSI0_BASE)
#define LM3S_SSI1     ((lm3s_ssi_t *)SSI1_BASE)

/*============================================================================
 *  I2C 模块
 *===========================================================================*/

#define I2C0_BASE         (PERIPH_BASE + 0x00003000U)
#define I2C1_BASE         (PERIPH_BASE + 0x00004000U)

typedef struct {
    volatile uint32_t MSA;          /* 0x000 - Master Slave Address */
    volatile uint32_t MCS;          /* 0x004 - Master Control/Status */
    volatile uint32_t MDR;          /* 0x008 - Master Data */
    volatile uint32_t MTPR;         /* 0x00C - Master Timer Period */
    volatile uint32_t MIMR;         /* 0x010 - Master Interrupt Mask */
    volatile uint32_t MRIS;         /* 0x014 - Master Raw Interrupt Status */
    volatile uint32_t MMIS;         /* 0x018 - Master Masked Interrupt Status */
    volatile uint32_t MICR;         /* 0x01C - Master Interrupt Clear */
    volatile uint32_t MCR;          /* 0x020 - Master Configuration */
} lm3s_i2c_t;

#define LM3S_I2C0     ((lm3s_i2c_t *)I2C0_BASE)
#define LM3S_I2C1     ((lm3s_i2c_t *)I2C1_BASE)

/*============================================================================
 *  定时器模块 (GPTM)
 *===========================================================================*/

#define TIMER0_BASE       (PERIPH_BASE + 0x00000000U)
#define TIMER1_BASE       (PERIPH_BASE + 0x00001000U)
#define TIMER2_BASE       (PERIPH_BASE + 0x00002000U)
#define TIMER3_BASE       (PERIPH_BASE + 0x00003000U)

typedef struct {
    volatile uint32_t CFG;          /* 0x000 - Configuration */
    volatile uint32_t TAMR;         /* 0x004 - Timer A Mode */
    volatile uint32_t TBMR;         /* 0x008 - Timer B Mode */
    volatile uint32_t CTL;          /* 0x00C - Control */
    volatile uint32_t SYNC;         /* 0x010 - Synchronize */
    volatile uint32_t RESERVED1[4];
    volatile uint32_t IMR;          /* 0x024 - Interrupt Mask */
    volatile uint32_t RIS;          /* 0x028 - Raw Interrupt Status */
    volatile uint32_t MIS;          /* 0x02C - Masked Interrupt Status */
    volatile uint32_t ICR;          /* 0x030 - Interrupt Clear */
    volatile uint32_t TAILR;        /* 0x034 - Timer A Interval Load */
    volatile uint32_t TBILR;        /* 0x038 - Timer B Interval Load */
    volatile uint32_t TAMATCHR;     /* 0x03C - Timer A Match */
    volatile uint32_t TBMATCHR;     /* 0x040 - Timer B Match */
    volatile uint32_t TAPR;         /* 0x044 - Timer A Prescale */
    volatile uint32_t TBPR;         /* 0x048 - Timer B Prescale */
    volatile uint32_t TAPMR;        /* 0x04C - Timer A Prescale Match */
    volatile uint32_t TBPMR;        /* 0x050 - Timer B Prescale Match */
    volatile uint32_t TAR;          /* 0x054 - Timer A Current Value */
    volatile uint32_t TBR;          /* 0x058 - Timer B Current Value */
} lm3s_timer_t;

#define LM3S_TIMER0   ((lm3s_timer_t *)TIMER0_BASE)
#define LM3S_TIMER1   ((lm3s_timer_t *)TIMER1_BASE)
#define LM3S_TIMER2   ((lm3s_timer_t *)TIMER2_BASE)
#define LM3S_TIMER3   ((lm3s_timer_t *)TIMER3_BASE)

/*============================================================================
 *  NVIC (中断控制器)
 *===========================================================================*/

#define NVIC_BASE         0xE000E000U

typedef struct {
    volatile uint32_t ISER[8];      /* 0x000 - Interrupt Set Enable */
    volatile uint32_t RESERVED1[24];
    volatile uint32_t ICER[8];      /* 0x080 - Interrupt Clear Enable */
    volatile uint32_t RESERVED2[24];
    volatile uint32_t ISPR[8];      /* 0x100 - Interrupt Set Pending */
    volatile uint32_t RESERVED3[24];
    volatile uint32_t ICPR[8];      /* 0x180 - Interrupt Clear Pending */
    volatile uint32_t RESERVED4[24];
    volatile uint32_t IABR[8];      /* 0x200 - Interrupt Active Bit */
    volatile uint32_t RESERVED5[56];
    volatile uint8_t  IPR[240];     /* 0x300 - Interrupt Priority */
} lm3s_nvic_t;

#define LM3S_NVIC     ((lm3s_nvic_t *)NVIC_BASE)

/*============================================================================
 *  SysTick 定时器
 *===========================================================================*/

#define SYSTICK_BASE      0xE000E010U

typedef struct {
    volatile uint32_t CTRL;         /* 0x000 - Control and Status */
    volatile uint32_t RELOAD;       /* 0x004 - Reload Value */
    volatile uint32_t CURRENT;      /* 0x008 - Current Value */
    volatile uint32_t CALIB;        /* 0x00C - Calibration Value */
} lm3s_systick_t;

#define LM3S_SYSTICK  ((lm3s_systick_t *)SYSTICK_BASE)

/* SysTick 控制位 */
#define SYSTICK_CTRL_ENABLE     0x00000001U
#define SYSTICK_CTRL_INTEN      0x00000002U
#define SYSTICK_CTRL_CLKSOURCE  0x00000004U

#ifdef __cplusplus
}
#endif

#endif /* __LM3S6965_H__ */
