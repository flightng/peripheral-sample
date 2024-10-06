/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-09-25     misonyo      first edition.
 */
/*
 * 程序清单：这是一个SD卡设备的使用例程
 * 例程导出了 sd_sample 命令到控制终端
 * 命令调用格式：sd_sample sd0
 * 命令解释：命令第二个参数是要使用的SD设备的名称，为空则使用例程默认的SD设备。
 * 程序功能：程序会产生一个块大小的随机数，然后写入SD卡中，然后在读取这部分写入的数据。
 *            对比写入和读出的数据是否一致，一致则表示程序运行正确。
 */

#include <rtdevice.h>
#include <rtthread.h>
#include <stdlib.h>

#define SD_DEVICE_NAME "sd0"

rt_device_t sd_device;
char sd_name[RT_NAME_MAX];

void fill_buffer(rt_uint8_t *buff, rt_uint32_t buff_length) {
  rt_uint32_t index;
  /* 往缓冲区填充随机数 */
  for (index = 0; index < buff_length; index++) {
    // buff[index] = ((rt_uint8_t)rand()) & 0xff;
    buff[index] = index;
  }
}

void test_secondRate(rt_uint16_t block_size,uint8_t times_second)
{
    uint16_t times = 0;
    rt_uint8_t *write_buff, *read_buff;
    rt_bool_t ret = RT_TRUE;
    rt_uint16_t block_num = 0;

    read_buff = rt_malloc(block_size);
    if (read_buff == RT_NULL)
    {
        rt_kprintf("no memory for read buffer!\n");
        return RT_ERROR;
    }
    write_buff = rt_malloc(block_size);
    if (write_buff == RT_NULL)
    {
        rt_kprintf("no memory for write buffer!\n");
        rt_free(read_buff);
        return RT_ERROR;
    }

    memset(read_buff, 0, block_size);

    uint32_t time_now = systime_now_ms();
    while (1)
    {
        block_num = rt_device_write(sd_device, times, write_buff, 1);
        if (1 != block_num)
        {
            rt_kprintf("write device %s failed!\n", sd_name);
            ret = RT_FALSE;
            break;
        }
        uint32_t time_after = systime_now_ms();
        uint32_t time_duing = time_after - time_now;
        if (time_duing > (1000 * times_second))
        {
            break;
        }
        else
        {
            times++;
        }
    }
    if (ret)
    {
        rt_kprintf("write device %s ok!,total %d times\n", sd_name, times);
    }
    else
    {
        rt_kprintf("write device %s failed!,total %d times\n", sd_name, times);
    }
}

void test_write_read(rt_uint16_t block_size, rt_uint16_t count) {
  rt_uint8_t *write_buff, *read_buff;
  rt_uint8_t block_num;
  rt_bool_t test_status = 1;

  read_buff = rt_malloc(block_size);
  if (read_buff == RT_NULL) {
    rt_kprintf("no memory for read buffer!\n");
    return RT_ERROR;
  }
  write_buff = rt_malloc(block_size);
  if (write_buff == RT_NULL) {
    rt_kprintf("no memory for write buffer!\n");
    rt_free(read_buff);
    return RT_ERROR;
  }

  memset(read_buff,0,block_size);

  for (rt_uint16_t i = 0; i < count; i++) {
    /* 填充写数据缓冲区，为写操作做准备 */
    fill_buffer(write_buff, block_size);

    /* 把写数据缓冲的数据写入SD卡中，大小为一个块，size参数以块为单位 */
    block_num = rt_device_write(sd_device, i, write_buff, 1);
    if (1 != block_num) {
      rt_kprintf("write device %s failed!\n", sd_name);
    }

    /* 从SD卡中读出数据，并保存在读数据缓冲区中 */
    block_num = rt_device_read(sd_device, i, read_buff, 1);
    if (1 != block_num) {
      rt_kprintf("read %s device failed!\n", sd_name);
    }

    /* 比较写数据缓冲区和读数据缓冲区的内容是否完全一致 */
    if (rt_memcmp(write_buff, read_buff, block_size) == 0) {
      rt_kprintf("Block test OK!,this is %d \n", i);
    } else {
      rt_kprintf("Block test Fail!,this is %d\n", i);  
      test_status = 0;    
      break;
    }
  }
  
  if(test_status)
  {
    rt_kprintf("Block test success!,test count = %d \n",count);
  }
  else
  {
    rt_kprintf("Block test fail!,test count = %d\n",count);

    rt_kprintf("this is write data\n");
    for(rt_uint16_t i = 0 ; i < block_size;i++)
    {
      rt_kprintf("%d ",write_buff[i]);
    }
    rt_kprintf("\n");
    rt_kprintf("this is read data\n");
    for(rt_uint16_t i = 0 ; i < block_size;i++)
    {
      rt_kprintf("%d ",read_buff[i]);
    }
  }

    /* 释放缓冲区空间 */
  rt_free(read_buff);
  rt_free(write_buff);
  
}

static int sd_sample(int argc, char *argv[]) {
  rt_err_t ret;
  rt_uint8_t *write_buff, *read_buff;
  struct rt_device_blk_geometry geo;
  rt_uint8_t block_num;
  rt_uint16_t test_count = 0 ;
  char test_mode[10];
  uint8_t test_mode_case = 0;

  if(argc != 4)
  {
    rt_kprintf("please enter sd0 test_count\n");
    return RT_ERROR;
  }

  rt_strncpy(sd_name, argv[1], RT_NAME_MAX);   
  rt_strncpy(test_mode, argv[2], 4);   
  test_count = atoi(argv[3]);

  if(strcmp(test_mode,"cout") == 0)
  {
    test_mode_case = 1;
  }
  else if(strcmp(test_mode,"time") == 0)
  {
    test_mode_case = 2;
  }
  else
  {
    rt_kprintf("test mode select error,please input cout/time");
    return RT_ERROR;
  }
  
  /* 查找设备获取设备句柄 */
  sd_device = rt_device_find(sd_name);
  if (sd_device == RT_NULL) {
    rt_kprintf("find device %s failed!\n", sd_name);
    return RT_ERROR;
  }
  /* 打开设备 */
  ret = rt_device_open(sd_device, RT_DEVICE_OFLAG_RDWR);
  if (ret != RT_EOK) {
    rt_kprintf("open device %s failed!\n", sd_name);
    return ret;
  }

  rt_memset(&geo, 0, sizeof(geo));
  /* 获取块设备信息 */
  ret = rt_device_control(sd_device, RT_DEVICE_CTRL_BLK_GETGEOME, &geo);
  if (ret != RT_EOK) {
    rt_kprintf("control device %s failed!\n", sd_name);
    return ret;
  }
  rt_kprintf("device information:\n");
  rt_kprintf("sector  size : %d byte\n", geo.bytes_per_sector);
  rt_kprintf("sector count : %d \n", geo.sector_count);
  rt_kprintf("block   size : %d byte\n", geo.block_size);

  switch (test_mode_case)
  {
  case 1:
      test_write_read(geo.block_size, test_count);
      break;
  case 2:
      test_secondRate(geo.block_size,test_count);
      break;
  default:
      break;
  }

  ret = rt_device_close(sd_device);
  if (ret != RT_EOK) {
    rt_kprintf("close device %s failed!\n", sd_name);
    return ret;
  } else {
    rt_kprintf("close device %s ok!\n", sd_name);
  }

  return RT_EOK;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(sd_sample, sd device sample);
