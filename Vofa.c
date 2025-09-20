/**
  ******************************************************************************
  * @file           : Vofa.c
  * @author         : Morrowan
  * @brief          : Vofa+上位机发送数据，支持FireWater和JustFloat两种发送格式
  * @attention      : 需要事先在STM32CubeMX中打开串口DMA以及串口全局中断，默认配置的最大发
                      送通道数为10，如果超过需要修改MAX_CHANNELS。
                      需要主函数初始化UART之后调用一次VOFA_Init再进行数据发送。
  ******************************************************************************
  */

#include "main.h"
#include "Vofa.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

/* 相关宏定义与变量声明 */
static const uint8_t FRAM_TAIL[4] = {0x00,0x00,0x80,0x7f};  //发送帧尾
#define MAX_CHANNELS 10   //要发送的数据通道个数
#define MAX_PREFIX_LEN 32   //支持的最长前缀
#define FRAM_TAIL_SIZE sizeof(FRAM_TAIL)    //帧尾大小
#define BUFFER_SIZE_JUSTFLOAT (MAX_CHANNELS*sizeof(float) + FRAM_TAIL_SIZE)   //JustFloat模式发送数据缓冲区大小
#define BUFFER_SIZE_FIREWATER (MAX_PREFIX_LEN + MAX_CHANNELS*sizeof(float))   //FireWater模式发送数据缓冲区大小
static uint8_t JustFloatDMATransmitBuffer[BUFFER_SIZE_JUSTFLOAT];    //JustFloat发送数据缓冲区
static char FireWaterDMATransmitBuffer[BUFFER_SIZE_FIREWATER];    //FireWater发送数据缓冲区
static UART_HandleTypeDef *SenderHUART = NULL;     //UART句柄

/* 发送初始化，完成UART句柄赋值
   传入参数：HUART通道
   返回值：无
*/
void VOFA_Init(UART_HandleTypeDef *huart) {
  SenderHUART = huart;
}

/* JustFloat发送函数
   传入参数：要发送的数据指针，需要发送的数据个数
   返回值：0->发送失败，1->发送成功
*/
uint8_t VOFA_Transmit_JustFloat(float *frame,uint8_t channel_count) {
  //数据发送合法性判断
  if (SenderHUART == NULL || frame == NULL || channel_count ==0 || channel_count > MAX_CHANNELS) {
    return 0;
  }
  //DMA状态判断
  if (HAL_DMA_GetState(SenderHUART->hdmatx)!= HAL_DMA_STATE_READY) {
    return 0;
  }
  //待发送数据大小
  uint16_t Datasize = channel_count * sizeof(float);
  //数据缓冲区写入
  memcpy(JustFloatDMATransmitBuffer, frame, Datasize);
  memcpy(JustFloatDMATransmitBuffer+Datasize, FRAM_TAIL, FRAM_TAIL_SIZE);
  //总发送数据大小计算
  uint16_t TotalSize = Datasize + FRAM_TAIL_SIZE;
  //DMA模式串口数据发送
  HAL_UART_Transmit_DMA(SenderHUART, (uint8_t *)JustFloatDMATransmitBuffer, TotalSize);
  return 1;
}

/* 格式化FireWater发送函数
   传入参数：类似于C语言的Printf，格式化打印
   返回值：0->发送失败，1->发送成功
*/
uint8_t VOFA_Transmit_FireWater(char* format,...) {
  //数据发送合法性判断
  if (SenderHUART == NULL || format == NULL || format[0] == '\0') {
    return 0;
  }
  //DMA状态判断
  if (HAL_DMA_GetState(SenderHUART->hdmatx)!= HAL_DMA_STATE_READY) {
    return 0;
  }
  //可变参数处理
  va_list args;
  va_start(args, format);
  //格式化字符串数据至数据缓冲区并返回发送字符数统计
  int TransmitBufferLen = vsnprintf(FireWaterDMATransmitBuffer, BUFFER_SIZE_FIREWATER, format, args);
  va_end(args);

  // 检查vsnprintf是否出错或字符串是否被截断
  if (TransmitBufferLen < 0 || TransmitBufferLen >= BUFFER_SIZE_FIREWATER) {
    // 格式化出错或缓冲区太小
    return 0;
  }

  //DMA模式串口数据发送
  HAL_UART_Transmit_DMA(SenderHUART, (uint8_t *)FireWaterDMATransmitBuffer, TransmitBufferLen);
  return 1;
}

/* 带描述词FireWater发送函数
   传入参数：描述词，数据，数据个数
   返回值：0->发送失败，1->发送成功
*/
uint8_t VOFA_Transmit_FireWater_Descriptor(char* prefix,float* frame,uint8_t channel_count) {
  //字符串格式化
  char* pBuffer = FireWaterDMATransmitBuffer;
  uint8_t RemainingSize = BUFFER_SIZE_FIREWATER;
  uint8_t WrittenSize = 0;
  //添加发送字符前缀
  if (prefix != NULL && prefix[0] != '\0') {
    WrittenSize = snprintf(pBuffer, RemainingSize, "%s", prefix);
    pBuffer += WrittenSize;
    RemainingSize -= WrittenSize;
  }
  //添加需要发送的浮点数
  for (uint8_t i = 0; i < channel_count; i++) {
    if (RemainingSize <= 1)   //数据缓冲区已满，退出添加
      break;
    //若是最后一个数，以"\n"结尾
    if (i == channel_count - 1) {
      WrittenSize = snprintf(pBuffer, RemainingSize, "%f\n", frame[i]);
    }
    //不是最后一个数以","结尾
    else {
      WrittenSize = snprintf(pBuffer, RemainingSize, "%f,", frame[i]);
    }
    //指针移位与缓冲区剩余大小更新
    pBuffer += WrittenSize;
    RemainingSize -= WrittenSize;
  }
  //总发送数据大小计算
  uint16_t TotalSize = strlen(FireWaterDMATransmitBuffer);
  //DMA模式串口数据发送
  HAL_UART_Transmit_DMA(SenderHUART, (uint8_t *)FireWaterDMATransmitBuffer, TotalSize);
  return 1;
}

