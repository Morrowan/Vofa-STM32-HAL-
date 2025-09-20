/**
  *******************************************************************************
  * @file           : Vofa.h
  * @author         : Morrowan
  * @brief          : Vofa+上位机发送数据，支持FireWater和JustFloat两种发送格式
  * @attention      : 需要事先在STM32CubeMX中打开串口DMA以及串口全局中断，默认配置的最大发
                      送个数为10，如果超过，需要修改Vofa.c内的宏定义MAX_CHANNELS。
                      需要主函数初始化UART之后调用一次VOFA_Init再进行数据发送。
                      FireWater提供两种发送模式：
                      1.VOFA_Transmit_FireWater，类似于C语言printf格式化发送，也可添加
                      描述词，类似于printf的写法，但是只可以在最初定义一个描述词；
                      2.VOFA_Transmit_FireWater_Descriptor，可配置一个描述词。
  ********************************************************************************
  */

#ifndef VOFA_H
#define VOFA_H

/**
 * @brief 初始化数据发送模块
 * @param huart 指向用于发送数据的UART句柄
 * @note  这个函数必须在主函数初始化UART之后被调用一次。
 */
void VOFA_Init(UART_HandleTypeDef *huart);

/**
 * @brief 使用DMA配合Vofa的JustFloat模式传输数据。
 * @param frame 指向要发送的浮点数数组的指针
 * @param channel_count 浮点数数组中的元素个数（通道数）
 * @return 如果成功启动DMA发送，返回1；如果DMA正忙，无法发送，则返回0。
 * @note 这是一个非阻塞函数。它会立即返回，数据会在后台通过DMA发送。
 * 如果返回0，意味着上一次的数据还没发完，本次的数据帧被丢弃。
 */
uint8_t VOFA_Transmit_JustFloat(float *frame,uint8_t channel_count);

/**
 * @brief 格式化字符串通过DMA配合Vofa的FireWater模式传输数据，功能类似printf。
 * @param format C语言风格的格式化字符串。
 * @param ...    要格式化的可变参数。
 * @return 如果成功启动DMA发送，返回1；如果DMA正忙或数据格式化出错，无法发送，则返回0。
 * @note 这是一个非阻塞函数，实现格式化串口打印。发送示例：VOFA_Transmit_FireWater("prefix:%f,%f",10.1f,10.3f);
 * @attention 为了在STM32上使用 %f (浮点数) 格式化，需要在项目设置中开启相关链接器选项。
 * 1.在STM32CubeIDE中: Project Properties -> C/C++ Build -> Settings -> MCU Settings -> 勾选 "Use float with printf from newlib-nano" ；
 * 2.在Keil中：Options_for_Target -> C/C++ -> Misc Controls -> 添加"--no-multibyte-chars" ;
 * 3.在GCC编译链：CMakeLists中添加“target_link_options(${PROJECT_NAME} PRIVATE -u _printf_float)” 。
 * 如果返回0，意味着上一次的数据还没发完或数据格式化出错，本次的数据帧被丢弃。
 * @attention 可以在发送最开始配置一个描述词，注意要以":"结尾，否则Vofa无法解析数据包。
 */
uint8_t VOFA_Transmit_FireWater(char* format,...);

/**
 * @brief 使用DMA配合Vofa的FireWater模式传输数据（带描述词）。
 * @param prefix 字符串前缀 (e.g., "samples:")。如果不需要前缀，可以传入NULL或者空字符串""。
 * @param frame 指向要发送的浮点数数组的指针。
 * @param channel_count 浮点数数组中的元素个数。
 * @return 如果成功启动DMA发送，返回1；如果DMA正忙，无法发送，则返回0。
 * @note  这是一个非阻塞函数。它会构建一个类似 "prefix:data1,data2,data3\n" 的字符串并启动DMA发送。
 * 如果返回0，意味着上一次的数据还没发完，本次的数据帧被丢弃。
 * @attention 描述词要以":"结尾，否则Vofa无法解析数据包。
 */
uint8_t VOFA_Transmit_FireWater_Descriptor(char* prefix,float* frame,uint8_t channel_count);

#endif //VOFA_H