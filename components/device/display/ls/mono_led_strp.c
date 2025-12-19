
/**
*   每个 seg 都是一个图层
*　 每个　seg 都有多个特效挂载
*   每个 seg 都共用一块存储，随着叠层慢慢更新
*   led 的数目为 16bit 已经够常规使用的了，可以使用 led_size_t 来定义
*/

int32_t set_led_max_num()
int32_t get_led_max_num()

g_led_max_num =30;
struct mono_seg {
	struct mono_seg parent_seg;
	struct mono_seg next_seg;
	uint16_t speed;
	uint16_t start_index;
	uint16_t led_num;
	uint8_t direction;
}
// 每个seg 都是一个图层

// 效果挂载，挂载点，挂载数目，
// 要去挂载函数需要判断不要内存越界了
struct effec{
	uint16_t start;
	uint16_t end;
	uint16_t loop_time; // 基本 0xffffff 为 forever 了
	uint32_t 
	// 不同效果需要不同点亮方式，亮1个，2个，或者间隔亮
	void *param;
}
某个效果实现，然后挂载到seg 某个index 上
int effect_xxx(start, end, step, )