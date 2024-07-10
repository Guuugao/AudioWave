#pragma once
#include <time.h>
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
 * @param seconds       录制时长, 0表示等待用户手动结束
 */
void record_audio_RAW(const char* filename, 
        unsigned int sample_rate, unsigned short bit_depth, 
        unsigned short channels, time_t duration);

/**
 * @brief 播放音频文件
 *
 * 播放指定的音频文件
 *
 * @param filename 输入文件名称
 */
void play_audio_RAW(const char* filename, 
        unsigned int sample_rate, unsigned short bit_depth, 
        unsigned short channels);

/**
 * @brief 中止播放或录制
 *
 * 当程序录制时，将停止录制，释放资源，保存文件
 * 程序播放时，将停止播放，释放资源
 *
 * @param sig   接收的信号，此处为 SIGINT(Ctrl + C)
 */
void handle_SIGINT(int sig);