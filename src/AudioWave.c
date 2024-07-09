#include "AudioWave.h"

/**
 * @brief 中止播放或录制
 *
 * 当程序录制时，将停止录制，释放资源，保存文件
 * 程序播放时，将停止播放，释放资源
 *
 * @param sig   接收的信号，此处为 SIGINT(Ctrl + C)
 */
void handle_SIGINT(int sig);

void set_PCM_hw_params(unsigned int sample_rate, unsigned short bit_depth, unsigned short channels);

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

/**
 * @brief 释放资源
 *
 * 重置标志位
 * 释放缓冲区
 * 释放设备
 * 关闭文件
 */
void release_resources();

static char *device = "plughw:0,0"; // 使用默认设备
static status_t status = IDLE; // 当前状态, -1-空闲 0-录制 1-播放
static int fd = -1;
static int stop = 0; // 标识是否停止录制或者播放
static char* buffer = NULL;
static snd_pcm_t *pcm = NULL;
static snd_pcm_sframes_t frames = 128; // 期望的周期帧数

void record_audio_RAW(const char* filename, 
        unsigned int sample_rate, unsigned short bit_depth, 
        unsigned short channels, time_t duration) {
    int record_frames = 0; // 记录实际上录制的帧数
    int err = 0; // alsa错误码
    int frame_size = (bit_depth / 8) * channels; // 每一帧的大小(字节)
    time_t start_time = 0;

    /* 打开文件 */
    if ((fd = open(filename, O_WRONLY | O_CREAT)) == -1){
    	fprintf(stderr, "Record - open file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    buffer = (char*)calloc(frames, frame_size); // 分配一个周期长度的缓冲区

 	/* 打开alsa设备, 模式为SND_PCM_STREAM_CAPTURE(录制) */
    if ((err = snd_pcm_open(&pcm, device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    	fprintf(stderr, "Record - open device: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    set_PCM_hw_params(sample_rate, bit_depth, channels);

    status = RECORD;
    start_time = time(NULL);
 	/* 写入音频数据 */
    do {
        /* 捕获数据 */
        record_frames = snd_pcm_readi(pcm, buffer, frames);
        if (record_frames == -EPIPE) { // EPIPE means overrun 
            fprintf(stderr, "Record - overrun occurred\n");
            snd_pcm_prepare(pcm);
        } else if (record_frames < 0) {
            fprintf(stderr, "Record - read data: %s\n", snd_strerror(record_frames));
            break;
        } else if (record_frames != (int)frames) {
            fprintf(stderr, "Record - short read, read %d frames\n", record_frames);
        }

        if(write(fd, buffer, record_frames * frame_size) == -1) {
            fprintf(stderr, "Record - write file\n");
            break;
        }
    } while ((duration <= 0 || (time(NULL) - start_time) < duration) && stop == 0);

    release_resources();
}

void play_audio_RAW(const char* filename, 
        unsigned int sample_rate, unsigned short bit_depth, 
        unsigned short channels) {
    int play_frames = 0; // 记录实际上播放的帧数量
    int read_bytes = 0; // 记录读取的字节数
    int err = 0; // alsa错误码
    int frame_size = (bit_depth / 8) * channels; // 每一帧的大小(字节)
    int start_time = 0;

    /* 打开文件 */
    if ((fd = open(filename, O_RDONLY)) == -1){
    	fprintf(stderr, "Play - open file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    buffer = (char*)calloc(frames, frame_size); // 分配一个周期长度的缓冲区

 	/* 打开alsa设备, 模式为SND_PCM_STREAM_CAPTURE(录制) */
    if ((err = snd_pcm_open(&pcm, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    	fprintf(stderr, "Play - open device: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    set_PCM_hw_params(sample_rate, bit_depth, channels);

    status = PLAYBACK;
    start_time = time(NULL);
    do {
        /* 从标准输入中获取数据 */
        read_bytes = read(fd, buffer, frames * frame_size);
        if (read_bytes == 0) {
            fprintf(stderr, "Play - end of file on input\n");
            break;
        } else if (read_bytes == -1) {
            fprintf(stderr, "Play - read file eoor\n");
            break;
        }

        /* 播放这些数据 */
        play_frames = snd_pcm_writei(pcm, buffer, read_bytes / frame_size);
        if (play_frames == -EPIPE) {
            /* EPIPE - underrun */
            fprintf(stderr, "Play - underrun occurred\n");
            snd_pcm_prepare(pcm);
        } else if (play_frames < 0) {
            fprintf(stderr, "Play - error from writei: %s\n", snd_strerror(play_frames));
            exit(EXIT_FAILURE);
        }  else if (play_frames != (int)frames) {
            fprintf(stderr, "Play - short write, write %d frames\n", play_frames);
        }
    } while (stop == 0);

    release_resources();
}

void handle_SIGINT(int sig){
    stop = 1;
}

void set_PCM_hw_params(unsigned int sample_rate, unsigned short bit_depth, unsigned short channels){
    int err = 0;
    unsigned int buffer_time = 1000000;      // 缓冲区长度(us)
    unsigned int period_time = 100000;       // 周期长度(us)
    snd_pcm_hw_params_t* params = NULL; // 硬件参数结构体

    /* 设置alsa设备硬件属性 */
    snd_pcm_hw_params_alloca(&params); // 从栈里分配硬件参数对象内存
	snd_pcm_hw_params_any(pcm, params); // 使用默认参数初始化参数对象
	snd_pcm_hw_params_set_rate_near(pcm, params, &sample_rate, 0); // 设置采样率
	snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED); // 设置交错模式
    snd_pcm_hw_params_set_format(pcm, params, get_alsa_format(bit_depth)); // 设置采样格式(样本长度, 有无符号, 大小端)
	snd_pcm_hw_params_set_channels(pcm, params, channels); // 设置通道数
    snd_pcm_hw_params_set_period_size_near(pcm, params, &frames, 0); // 设置通道数
    snd_pcm_hw_params_set_buffer_time_near(pcm, params, &buffer_time, 0); // 设置缓存大小
    snd_pcm_hw_params_set_period_time_near(pcm, params, &period_time, 0); // 设置周期大小

    if ((err = snd_pcm_hw_params(pcm, params)) < 0) { // 把上述设置值写入设备
        fprintf(stderr, "Record - write params: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    if ((err = snd_pcm_hw_params_get_period_size(params, &frames, 0)) < 0){ // 获取实际周期大小
        fprintf(stderr, "Record - get period: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
	}
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

int is_small_endian(){  
    int a = 0x1;
    return *(char*)&a;
}

void release_resources(){
    status = IDLE;
    stop = 0;
    snd_pcm_drain(pcm);
    snd_pcm_close(pcm);
    free(buffer);
    if (close(fd) == -1) {
        fprintf(stderr, "close file fail\n");
    }
}