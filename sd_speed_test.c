#include <dfs_file.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <stdlib.h>

#define SD_DEVICE_NAME "sd0"
#define PIN_DEBUG_SD_SAMPLE 0
#define TIME_SECONE 1

static rt_device_t sd_device;

static void fill_buffer_index(rt_uint8_t *buff, rt_uint32_t buff_length, uint8_t index_check) {
    rt_uint32_t index;

    for (index = 0; index < buff_length; index++) {
        buff[index] = index + index_check;
    }
}

int sd_write_speed_test(rt_uint16_t sector_size, uint16_t serctor_count) {
    uint16_t times = 0;
    rt_uint8_t *write_buff, *read_buff;
    rt_uint16_t block_num = 0;
    rt_uint16_t opt_length_pre_times = sector_size * serctor_count;
    rt_bool_t result = 1;

    read_buff = rt_malloc(opt_length_pre_times);
    if (read_buff == RT_NULL) {
        rt_free(read_buff);
        rt_kprintf("no memory for read buffer!\n");
        return -1;
    }
    write_buff = rt_malloc(opt_length_pre_times);
    if (write_buff == RT_NULL) {
        rt_kprintf("no memory for write buffer!\n");
        rt_free(write_buff);
        return -1;
    }
    uint32_t time_now = rt_tick_get();
    while (1) {
        fill_buffer_index(write_buff, opt_length_pre_times, times);
        block_num = rt_device_write(sd_device, times, write_buff, serctor_count);
        if (serctor_count != block_num) {
            rt_kprintf("write device %s failed!\n", SD_DEVICE_NAME);
            // break;
        }
        uint32_t time_after = rt_tick_get();
        uint32_t time_duing = time_after - time_now;
        if (time_duing > (1000 * TIME_SECONE)) {
            break;
        } else {
            times += serctor_count;
        }
    }

    uint32_t total_byte = times * sector_size;
    uint16_t speed = (total_byte / 1024) / TIME_SECONE;
    uint32_t bps = speed * 1024 * 10;
    rt_kprintf("write device over!\r\n");
    rt_kprintf("per write block is %d \r\n", opt_length_pre_times);
    rt_kprintf("total %d times %d byte,in %d s\n", times, total_byte, TIME_SECONE);
    rt_kprintf("speed %dKB/s, baudrate %d\r\n", speed, bps);

    uint16_t read_size = 0;
    for (uint16_t i = 0; i < times; i += serctor_count) {
        memset(read_buff, 0, opt_length_pre_times);
        read_size = rt_device_read(sd_device, i, read_buff, serctor_count);
        if (read_size != serctor_count) {
            rt_kprintf("read fail in %d times,ret is %d\r\n", i, read_size);
            // break;
        }
        fill_buffer_index(write_buff, opt_length_pre_times, i);
        if (rt_memcmp(write_buff, read_buff, opt_length_pre_times) == 0) {
            // rt_kprintf("read check ok,in %d times\r\n", i + 1);
        } else {
            rt_kprintf("read check fail,in %d times\r\n", i + 1);
            result = 0;
            break;
        }
    }
    if (result) {
        rt_kprintf("all block test success\r\n");
    }

    rt_free(write_buff);
    rt_free(read_buff);
}

static int sd_speed_test(int argc, char *argv[]) {
    rt_err_t ret;
    struct rt_device_blk_geometry geo;
    uint16_t sector_count;

    if (argc != 2) {
        rt_kprintf("please enter sd0 test_count\n");
        return -1;
    }

    sector_count = atoi(argv[1]);

    /* 查找设备获取设备句柄 */
    sd_device = rt_device_find(SD_DEVICE_NAME);
    if (sd_device == RT_NULL) {
        rt_kprintf("find device %s failed!\n", SD_DEVICE_NAME);
        return -1;
    }
    /* 打开设备 */
    ret = rt_device_open(sd_device, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK) {
        rt_kprintf("open device %s failed!\n", SD_DEVICE_NAME);
        return ret;
    }
    rt_memset(&geo, 0, sizeof(geo));
    /* 获取块设备信息 */
    ret = rt_device_control(sd_device, RT_DEVICE_CTRL_BLK_GETGEOME, &geo);
    if (ret != RT_EOK) {
        rt_kprintf("control device %s failed!\n", SD_DEVICE_NAME);
        return ret;
    }
    rt_kprintf("device information:\n");
    rt_kprintf("sector  size : %d byte\n", geo.bytes_per_sector);
    rt_kprintf("sector count : %d \n", geo.sector_count);
    rt_kprintf("memory  size : %d MB\n", geo.sector_count / 1024 / 1024);

#if PIN_DEBUG_SD_SAMPLE
    test_pinToggle1();
#endif
    sd_write_speed_test(geo.bytes_per_sector, sector_count);
#if PIN_DEBUG_SD_SAMPLE
    test_pinToggle1();
#endif

    ret = rt_device_close(sd_device);
    if (ret != RT_EOK) {
        rt_kprintf("close device %s failed!\n", SD_DEVICE_NAME);
        return ret;
    } else {
        rt_kprintf("close device %s ok!\n", SD_DEVICE_NAME);
    }

    return RT_EOK;
}

MSH_CMD_EXPORT(sd_speed_test, sd device sample);
