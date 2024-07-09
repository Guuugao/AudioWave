#include "AudioWave.h"

static char *device = "hw:0,0";    // 使用默认设备

void record_audio(const char* filename, 
        unsigned int sample_rate, unsigned short bit_depth, 
        unsigned short channels, unsigned int duration) {
    int rc = 0; // 记录实际上录制的帧数
    int err = 0; // alsa错误码
    int out_fd = -1; // 输出文件描述符
    char* buffer = NULL;
    int frame_size = (bit_depth / 8) * channels; // 每一帧的大小(字节)
    int buffer_time = 1000000;       // 缓冲区长度(us)
    int period_time = 100000;       // 周期长度(us)
    snd_pcm_t *pcm = NULL; // 
    snd_pcm_sframes_t frames = 128; // 期望的周期帧数
    snd_pcm_hw_params_t* params = NULL; // 硬件参数结构体

    /* 打开文件 */
    if ((out_fd = open(filename, O_WRONLY | O_CREAT)) == -1){
    	fprintf(stderr, "Record - open file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    buffer = (char*)calloc(frames, frame_size); // 分配一个周期长度的缓冲区

 	/* 打开alsa设备, 模式为SND_PCM_STREAM_CAPTURE(录制) */
    if ((err = snd_pcm_open(&pcm, device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    	fprintf(stderr, "Record - open device: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    /* 设置alsa设备硬件属性 */
    snd_pcm_hw_params_alloca(&params); // 从栈里分配硬件参数对象内存
	if ((err = snd_pcm_hw_params_any(pcm, params)) < 0) { // 使用默认参数初始化参数对象
    	fprintf(stderr, "Record - fill params: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
	if ((err = snd_pcm_hw_params_set_rate_near(pcm, params, &sample_rate, 0)) < 0){ // 设置采样率
    	fprintf(stderr, "Record - set rate: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
	if ((err = snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0){ // 设置交错模式
        fprintf(stderr, "Record - set access: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
	}
    if ((err = snd_pcm_hw_params_set_format(pcm, params, get_alsa_format(bit_depth))) < 0){ // 设置采样格式(样本长度, 有无符号, 大小端)
        fprintf(stderr, "Record - set format: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
	if ((err = snd_pcm_hw_params_set_channels(pcm, params, channels)) < 0){ // 设置通道数
        fprintf(stderr, "Record - set channels: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
	}
    if ((err = snd_pcm_hw_params_set_period_size_near(pcm, params, &frames, 0)) < 0){ // 设置通道数
        fprintf(stderr, "Record - set period size: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
	}
    if ((err = snd_pcm_hw_params_set_buffer_time_near(pcm, params, &buffer_time, 0)) < 0){ // 设置缓存大小
        fprintf(stderr, "Record - set buffer time: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
	}
    if ((err = snd_pcm_hw_params_set_period_time_near(pcm, params, &period_time, 0)) < 0){ // 设置周期大小
        fprintf(stderr, "Record - set buffer time: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
	}
    if ((err = snd_pcm_hw_params(pcm, params)) < 0){ // 把上述设置值写入设备
        fprintf(stderr, "Record - write params: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
 
    if ((err = snd_pcm_hw_params_get_period_size(params, &frames, 0)) < 0){ // 获取实际周期大小
        fprintf(stderr, "Record - get period: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
	}

    int val = 0;
    snd_pcm_hw_params_get_period_time(params, &val, 0);
    printf("Record - frames: %ld, frame size: %d\n", frames, frame_size);

 	/* 写入音频数据 */
    for (int loops = 5000000 / val; loops > 0; --loops) {
        /* 捕获数据 */
        rc = snd_pcm_readi(pcm, buffer, frames);
        if (rc == -EPIPE) { // EPIPE means overrun 
            fprintf(stderr, "Record - overrun occurred\n");
            snd_pcm_prepare(pcm);
        } else if (rc < 0) {
            fprintf(stderr, "Record - read data: %s\n", snd_strerror(rc));
            break;
        } else if (rc != (int)frames) {
            fprintf(stderr, "Record - short read, read %d frames\n", rc);
        }

        if(write(out_fd, buffer, rc * frame_size) == -1) {
            fprintf(stderr, "Record - write file\n");
            break;
        }
    }
 
	/* 关闭声卡设备 */
    snd_pcm_drain(pcm); // 停止保存待处理帧
    snd_pcm_close(pcm); // 关闭设备
    free(buffer);
    if (close(out_fd) == -1){
    	fprintf(stderr, "Record - close file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void play_audio(const char* filename) {
    int bit_depth = 16;
    int channels = 2;
    int sample_rate = 44100;

    int rc = 0; // 记录实际上录制的帧数
    int err = 0; // alsa错误码
    int in_fd = -1; // 输出文件描述符
    char* buffer = NULL;
    int frame_size = (bit_depth / 8) * channels; // 每一帧的大小(字节)
    snd_pcm_t *pcm = NULL; // 
    snd_pcm_sframes_t frames = 128; // 期望的周期帧数
    snd_pcm_hw_params_t* params = NULL; // 硬件参数结构体

    /* 打开文件 */
    if ((in_fd = open(filename, O_RDONLY)) == -1){
    	fprintf(stderr, "Play - open file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    buffer = (char*)calloc(frames, frame_size); // 分配一个周期长度的缓冲区

 	/* 打开alsa设备, 模式为SND_PCM_STREAM_CAPTURE(录制) */
    if ((err = snd_pcm_open(&pcm, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    	fprintf(stderr, "Play - open device: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    /* 设置alsa设备硬件属性 */
    snd_pcm_hw_params_alloca(&params); // 从栈里分配硬件参数对象内存
	if ((err = snd_pcm_hw_params_any(pcm, params)) < 0) { // 使用默认参数初始化参数对象
    	fprintf(stderr, "Play - alloca params: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
	if ((err = snd_pcm_hw_params_set_rate_near(pcm, params, &sample_rate, 0)) < 0){ // 设置采样率
    	fprintf(stderr, "Play - fill params: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
	if ((err = snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0){ // 设置交错模式
        fprintf(stderr, "Play - set params: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
	}
    if ((err = snd_pcm_hw_params_set_format(pcm, params, get_alsa_format(bit_depth))) < 0){ // 设置采样格式(样本长度, 有无符号, 大小端)
        fprintf(stderr, "Play - set params: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
	if ((err = snd_pcm_hw_params_set_channels(pcm, params, channels)) < 0){ // 设置通道数
        fprintf(stderr, "Play - set params: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
	}
    if ((err = snd_pcm_hw_params_set_period_size_near(pcm, params, &frames, 0)) < 0){ // 设置通道数
        fprintf(stderr, "Play - set params: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
	}
    if ((err = snd_pcm_hw_params(pcm, params)) < 0){ // 把上述设置值写入设备
        fprintf(stderr, "Play - write params: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
 
    if ((err = snd_pcm_hw_params_get_period_size(params, &frames, 0)) < 0){ // 获取实际周期大小
        fprintf(stderr, "Play - get period: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
	}

    rc = (read(in_fd, buffer, frames * frame_size) / frame_size);
    while (rc >= 0) {
        /* 从标准输入中获取数据 */
        if (rc == 0) {
            fprintf(stderr, "Play - end of file on input\n");
            break;
        } else if (rc != frames) {
            fprintf(stderr,
                "Play - short read: read %d frame\n", rc);
        }

        /* 播放这些数据 */
        rc = snd_pcm_writei(pcm, buffer, rc);
        if (rc == -EPIPE) {
            /* EPIPE - underrun */
            fprintf(stderr, "Play - underrun occurred\n");
            snd_pcm_prepare(pcm);
        } else if (rc < 0) {
            fprintf(stderr, "Play - error from writei: %s\n", snd_strerror(rc));
            exit(EXIT_FAILURE);
        }  else if (rc != (int)frames) {
            fprintf(stderr, "Play - short write, write %d frames\n", rc);
        }
        rc = (read(in_fd, buffer, frames * frame_size) / frame_size);
    }

    snd_pcm_drain(pcm);
    snd_pcm_close(pcm);
    free(buffer);
    close(in_fd);
}

snd_pcm_format_t get_alsa_format(short bit_depth) {
    switch (bit_depth) {
        case 8:
            return SND_PCM_FORMAT_U8;
        case 16:
            return is_small_endian() ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_S16_BE;
        case 24:
            return is_small_endian() ? SND_PCM_FORMAT_S24_LE : SND_PCM_FORMAT_S24_BE;
        case 32:
            return is_small_endian() ? SND_PCM_FORMAT_S32_LE : SND_PCM_FORMAT_S32_BE;
        default:
            fprintf(stderr, "Unsupported bit depth: %d\n", bit_depth);
            return SND_PCM_FORMAT_UNKNOWN;
    }
}

// 判断机器是否为小端字节序
int is_small_endian(){
    int a = 0x1;
    return *(char*)&a;
}