#pragma once
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <alsa/asoundlib.h>

/**
 * @brief 根据给定参数进行音频录制
 *
 * 该函数使用指定参数录制音频并保存为文件
 * 可以指定位深, 采样率, 声道数
 *
 * @param filename      导出的音频文件名称
 * @param sample_rate   采样率(Hz) (e.g., 8000，44100, 96000).
 * @param bit_depth     位深 (e.g., 16, 24, 32).
 * @param channels      通道数 (e.g., 1 - 单声道, 2 - 双声道).
 * @param seconds       录制时长
 */
void record_audio(const char* filename, 
        unsigned int sample_rate, unsigned short bit_depth, 
        unsigned short channels, unsigned int duration);

/**
 * @brief 播放音频文件
 *
 * 播放指定的音频文件
 *
 * @param filename 输入文件名称
 */
void play_audio(const char* filename);

/**
 * @brief 转换参数
 *
 * 转换用户输入的位深为alsa库枚举
 *
 * @param bit_depth     用户输入的位深
 */
snd_pcm_format_t get_alsa_format(short bit_depth);

/**
 * @brief 判断机器字节序
 *
 * 该函数用于检查系统的字节序。 
 * 在小端字节序中，最小有效字节（LSB）存储在最小的内存地址，最大有效字节（MSB）存储在最大的内存地址。 
 * 在大端字节序中，顺序则相反。
 *
 * @param bit_depth     用户输入的位深
 * @return 如果是小端字节序则返回1，否则返回0
 */
int is_small_endian();