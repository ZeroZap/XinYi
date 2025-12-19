#include "xy_mem.h"


// #error "xy_enter_critical() and xy_exit_critical() need to be defined for
// your platform."

typedef struct _xy_mem {
    xy_size_t mem_size;
    // 8051中xdata的指针为2byte，所以sizeof(xy_mem_t) = 4
    struct _xy_mem *next;
} xy_mem_t;


xy_mem_t *g_mem = NULL;

// 16bit = 65535 = 64kByte, enough
void xy_mem_set(void *p_data, xy_uint8_t set_data, xy_size_t mem_size)
{
    xy_uint8_t *p = (xy_uint8_t *)p_data;
    while (mem_size--)
        *p++ = set_data;
}

xy_int8_t xy_mem_cmp(const void *str1, const void *str2, xy_uint8_t len)
{
    xy_uint8_t *s = (xy_uint8_t *)str1;
    xy_uint8_t *d = (xy_uint8_t *)str2;
    xy_uint8_t i;
    for (i = 0; i < len; i++) {
        if ((*(s + i)) < (*(d + i)))
            return -1;
        if ((*(s + i)) > (*(d + i)))
            return 1;
    }
    return 0;
}

void xy_mem_copy(void *p_dest_data, void *p_src_data, xy_size_t mem_size)
{
    xy_uint8_t *s = (xy_uint8_t *)p_src_data;
    xy_uint8_t *d = (xy_uint8_t *)p_dest_data;


    /**源超前，指针递增不会覆盖*/
    if (p_dest_data < p_src_data) {
        while (mem_size--)
            *d++ = *s++;
    } else {           /**目的超前，要指针递减，避免源被覆盖*/
        s += mem_size; // 超前了一个 mem_size
        d += mem_size; // 超前了一个 mem_size
        while (mem_size--)
            *--d = *--s;
    }
}

/**
 * @brief
 * 从空闲的内存块中获取，仅在 mem_size < p->size
 * @param p
 * @param mem_size
 * @return void*
 */

static void *mem_break(xy_mem_t *p, xy_size_t mem_size)
{
    /* 从p+mem_size后拟造一个节点，切开 */
    xy_mem_t *d = (xy_mem_t *)((xy_uint8_t *)p + mem_size);
    // 获取P 剩余的mem_size
    d->mem_size = p->mem_size - mem_size;
    // 继承p->next
    d->next = p->next;
    // p的mem_size也减少了，那p->next怎么办？
    p->mem_size = mem_size;
    return d;
}

static void mem_merge(xy_mem_t *p1, xy_mem_t *p2)
{
    // if p1->next == p2 查看是不是物理连接在一起了
    if ((xy_mem_t *)((xy_uint8_t *)p1 + p1->mem_size) == p2) {
        p1->mem_size += p2->mem_size;
        p1->next = p2->next;
    } else // 如果不是合并，那么就是要跳转了
    {
        p1->next = p2;
    }
}

void xy_mem_init(void *p, xy_size_t mem_size)
{
    xy_mem_t *h;
    h           = (xy_mem_t *)p;
    h->mem_size = mem_size;
    h->next     = NULL;
    g_mem       = h;
}


void *xy_mem_malloc(xy_size_t mem_size)
{
    void *p;
    xy_enter_critical();
    p = xy_mem_malloc_from_irq(mem_size);
    xy_exit_critical();
    return p;
}


void *xy_mem_malloc_from_irq(xy_size_t mem_size)
{ // 理解为 int *p与p
    xy_mem_t **pp, *p;

    // add size for xy_mem_t->size
    mem_size += sizeof(xy_size_t);

    // 指针内存对齐，align up 算法： (((size) + (align) - 1) & ~((align) - 1))
    mem_size =
        (((mem_size) + (sizeof(xy_size_t)) - 1) & ~((sizeof(xy_size_t)) - 1));

    if (mem_size < sizeof(xy_mem_t))
        mem_size = sizeof(xy_mem_t);
    /** 查看空内存块是否有适合的可分配空间
     * 1、pp 获取指向内存第一个内存块指针地址（&g_mem）
     * 2、p=*pp, p=g_mem, pp移动了，g_mem也就移动辣
     * 3、当p->size不够分配时，pp获取指向p->next指针地址（也就是指向下一内存卡的指针地址）
     * 4、当 size 够时，*pp=
     * 也就是当前内存块next存储地址要变更了（就是指向mem_t变更了）
     */
    for (pp = (&g_mem); ((p = *pp) != NULL); pp = &p->next) {
        if (mem_size <= p->mem_size) // 还有内存
        {
            /**
             * *pp 更改指针pp的内容了
             */
            *pp = (p->mem_size >= mem_size + sizeof(xy_mem_t))
                      ? mem_break(p, mem_size)
                      : p->next;
            return (&p->next); // 返回p->next所在的地址
        }
    }
    return NULL;
}


void xy_mem_free(void *p)
{
    xy_enter_critical();
    xy_mem_free_from_irq(p);
    xy_exit_critical();
}

void xy_mem_free_from_irq(void *p)
{
    xy_mem_t *x, *y, *z;

    // p = xy_mem_t->next，p->next - p->size,就是p所在的mem_t啦
    y = (xy_mem_t *)((xy_uint8_t *)p - sizeof(xy_size_t));

    if (!g_mem) {
        //        printf("g_mem = NULL\n");
        y->next = NULL;
        g_mem   = y;
    } else if (y < g_mem) {
        //        printf("y<g_mem\n");
        mem_merge(y, g_mem);
        g_mem = y;
    } else {
        //        printf("y>=g_mem\n");
        // 查找释放mem_t上一个mem_t，z为 xy_mem_t->next
        for (x = g_mem; (((z = x->next) != NULL) && (z < y)); x = z)
            ;
        // 如果 z 不为空，则与 z 拼接一起
        if (z) {
            //            printf("if(z)\n");
            mem_merge(y, z);
        } else //
        {
            // z = null
            //            printf("z=NULL\n");
            y->next = NULL;
        }
        mem_merge(x, y);
    }
}

xy_uint16_t xy_mem_info(xy_uint16_t *p_num, xy_uint16_t *p_max)
{
    xy_uint16_t mem_size = 0;
    xy_uint16_t num      = 0;
    xy_uint16_t max      = 0;

    xy_mem_t *p;
    xy_enter_critical();
    for (p = g_mem; p; p = p->next) {
        mem_size += p->mem_size;
        if (max < p->mem_size)
            max = p->mem_size;
        num++;
    }
    xy_exit_critical();
    *p_num = num;
    *p_max = max;
    return mem_size;
}

#if (PLATFORM == PLATFORM_X86)

void main(void)
{
    xy_uint16_t g_ram_a[1024];
    xy_uint16_t mem_block_num, mem_max, mem_size;
    xy_mem_t *taska, *taskb, *taskc, *taskd;

    printf("sizeof xy_mem_t:%d\n", sizeof(xy_mem_t));

    xy_mem_init(g_ram_a, sizeof(g_ram_a));
    mem_size = xy_mem_info(&mem_block_num, &mem_max);
    printf("After init mem_size=%d\n", mem_size);

    taska    = xy_mem_malloc(256);
    mem_size = xy_mem_info(&mem_block_num, &mem_max);
    printf("after malloc mem_size=%d\n", mem_size);

    taskb    = xy_mem_malloc(128);
    mem_size = xy_mem_info(&mem_block_num, &mem_max);
    printf("after malloc mem_size=%d\n", mem_size);

    taskc    = xy_mem_malloc(50);
    mem_size = xy_mem_info(&mem_block_num, &mem_max);
    printf("after malloc, mem_size=%d\n", mem_size);

    xy_mem_free(taskb);
    mem_size = xy_mem_info(&mem_block_num, &mem_max);
    printf("Atfer free mem_size=%d\n", mem_size);

    taskd    = xy_mem_malloc(24);
    mem_size = xy_mem_info(&mem_block_num, &mem_max);
    printf("Atfer malloc mem_size=%d\n", mem_size);

    xy_mem_free(taska);
    mem_size = xy_mem_info(&mem_block_num, &mem_max);
    printf("Atfer free mem_size=%d\n", mem_size);

    xy_mem_free(taskd);
    mem_size = xy_mem_info(&mem_block_num, &mem_max);
    printf("Atfer free mem_size=%d\n", mem_size);

    xy_mem_free(taskc);
    mem_size = xy_mem_info(&mem_block_num, &mem_max);
    printf("Atfer free mem_size=%d\n", mem_size);
}

#elif (PLATFORM == PLATFORM_C51)

#if 1
// volatile xy_uint8_t g_ram_a[256];
// void test_mem(void)
//{

//  xy_uint16_t mem_block_num, mem_max, mem_size;
//  xy_mem_t *taska, *taskb, *taskc, *taskd;

////  printf("sizeof xy_mem_t:%d\n", sizeof(xy_mem_t));

////  xy_mem_init(g_ram_a, sizeof(g_ram_a));
//  mem_size = xy_mem_info(&mem_block_num, &mem_max);
////  printf("After init mem_size=%d\n", mem_size);

//  taska = xy_mem_malloc(32);
//  mem_size = xy_mem_info(&mem_block_num, &mem_max);
////  printf("after malloc mem_size=%d\n", mem_size);

//  taskb = xy_mem_malloc(46);
//  mem_size = xy_mem_info(&mem_block_num, &mem_max);
////  printf("after malloc mem_size=%d\n", mem_size);

//  taskc = xy_mem_malloc(24);
//  mem_size = xy_mem_info(&mem_block_num, &mem_max);
////  printf("after malloc, mem_size=%d\n", mem_size);

//  xy_mem_free(taskb);
//  mem_size = xy_mem_info(&mem_block_num, &mem_max);
////  printf("Atfer free mem_size=%d\n", mem_size);

//  taskd = xy_mem_malloc(40);
//  mem_size = xy_mem_info(&mem_block_num, &mem_max);
////  printf("Atfer malloc mem_size=%d\n", mem_size);

//  xy_mem_free(taska);
//  mem_size = xy_mem_info(&mem_block_num, &mem_max);
////  printf("Atfer free mem_size=%d\n", mem_size);

//  xy_mem_free(taskd);
//  mem_size = xy_mem_info(&mem_block_num, &mem_max);
////  printf("Atfer free mem_size=%d\n", mem_size);

//  xy_mem_free(taskc);
//  mem_size = xy_mem_info(&mem_block_num, &mem_max);
////  printf("Atfer free mem_size=%d\n", mem_size);
//}
#endif

#endif
