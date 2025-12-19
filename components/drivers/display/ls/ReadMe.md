





## 结构体

### led

```c
struct led{
#if LED_TYPE == TYPE_MONO_RGB
    uint8_t RGB[3];
#endif
#if  LED_TYPE == TYPE_MONO_RGBW
    uint8_t RGBW[4];
#endif
	uint8_t brightness;
}


struct led_strip{
    struct led *raw_data;
    uint16_t 
    uint16_t start;
    uint16_t end;
    uint16_t speed;
}
```











led_set_rgb(struct led,  r,  g,  b,  brightness) 

led_set_rgbw()



## API

```c
mono_xxx()
rgb_set_color()
rgbw_set_color()
chip_set_color()
chip_set_rgb(r,g,b)
chip_set_rgbw(r,g,b,w)    
```





### 基础用法

```c
init()
setspeed
setcolor
setmode
start
```





### effect

```c
struct effect {
    uint8_t mode; // loop mode
    uint8_t group; // 所属组，效果过后切换到下一个
}
```



### 基础效果

```c
/**

*/
uint16_t blink(uint32_t color1, uint32_color2, uint8_t strobe)
{
    
}

/**
 - reverse 反序输出
*/
uint16_t color_wipe(uint32_t color1, uint32_t color2, uint8_t reverse)
{
    
}
```









## 移植接口

### 单色的好说

### RGB也分chip 还是 pwm



