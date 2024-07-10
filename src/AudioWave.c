#include "AudioWave.h"

#define DEFAULT_DEVICE "plughw:0,0" // 默认设备
#define DEFAULT_FRAMES 128  // 一个周期的默认帧数
static int stop_flag = 0;   // 标识是否停止录制或者播放

// typedef enum {
//     IDLE = -1,  // 空闲
//     RECORD,     // 录制
//     PLAYBACK    // 播放
// }status_t;

typedef struct {
    int fd;
    char* buffer;
    snd_pcm_t *pcm;
    snd_pcm_sframes_t exp_frames;   // 期望的周期帧数
    snd_pcm_sframes_t act_frames;   // 记录实际上操作的帧数
    unsigned int frame_size;        // 每一帧的大小(字节)
}audio_data_t;

void set_PCM_hw_params(audio_data_t* audio_data, unsigned int sample_rate, unsigned short bit_depth, unsigned short channels);

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
 * 重置标志位|释放缓冲区|释放设备|关闭文件
 */
void release_resources(audio_data_t* audio_data);

/**
 * @brief 返回格式化时间字符串
 * 
 * 根据传入的秒数，将时间格式化为hh:mm:ss的格式
 * 
 * @param seconds     间隔的秒数
 * @return 格式化的时间字符串
 */
char* format_time_interval(int seconds);

void record_audio_RAW(const char* filename, 
        unsigned int sample_rate, unsigned short bit_depth, 
        unsigned short channels, time_t duration) {
    int err = 0; // alsa错误码
    time_t start_time = 0;
    audio_data_t audio_data = { 0 };

    audio_data.fd = -1;
    audio_data.buffer = NULL;
    audio_data.pcm = NULL;
    audio_data.exp_frames = DEFAULT_FRAMES;
    audio_data.act_frames = 0;
    audio_data.frame_size = (bit_depth / 8) * channels;

 	/* 打开alsa设备, 模式为SND_PCM_STREAM_CAPTURE(录制) */
    if ((err = snd_pcm_open(&audio_data.pcm, DEFAULT_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    	fprintf(stderr, "Record - open device: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    set_PCM_hw_params(&audio_data, sample_rate, bit_depth, channels);
    /* 打开文件 */
    if ((audio_data.fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC)) == -1){
    	fprintf(stderr, "Record - open file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    audio_data.buffer = (char*)calloc(audio_data.exp_frames, audio_data.frame_size); // 分配一个周期长度的缓冲区

    start_time = time(NULL);
 	/* 写入音频数据 */
    do {
        /* 捕获数据 */
        audio_data.act_frames = snd_pcm_readi(audio_data.pcm, audio_data.buffer, audio_data.exp_frames);
        if (audio_data.act_frames == -EPIPE) { // EPIPE - overrun 
            fprintf(stderr, "Record - overrun occurred\n");
            snd_pcm_prepare(audio_data.pcm);
        } else if (audio_data.act_frames < 0) {
            fprintf(stderr, "Record - read data: %s\n", snd_strerror(audio_data.act_frames));
            break;
        } else if (audio_data.act_frames != audio_data.exp_frames) {
            fprintf(stderr, "Record - short read, read %ld frames\n", audio_data.act_frames);
        }

        if(write(audio_data.fd, audio_data.buffer, audio_data.act_frames * audio_data.frame_size) == -1) {
            fprintf(stderr, "Record - write file\n");
            break;
        }
    } while ((duration <= 0 || (time(NULL) - start_time) < duration) && stop_flag == 0);

    release_resources(&audio_data);
}

void play_audio_RAW(const char* filename, 
        unsigned int sample_rate, unsigned short bit_depth, 
        unsigned short channels) {
    int err = 0; // alsa错误码
    int read_bytes = 0; // 记录读取的字节数
    int start_time = 0;
    audio_data_t audio_data = { 0 };

    audio_data.fd = -1;
    audio_data.buffer = NULL;
    audio_data.pcm = NULL;
    audio_data.exp_frames = DEFAULT_FRAMES;
    audio_data.act_frames = 0;
    audio_data.frame_size = (bit_depth / 8) * channels;

 	/* 打开alsa设备, 模式为SND_PCM_STREAM_CAPTURE(录制) */
    if ((err = snd_pcm_open(&audio_data.pcm, DEFAULT_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    	fprintf(stderr, "Play - open device: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    set_PCM_hw_params(&audio_data, sample_rate, bit_depth, channels);
    /* 打开文件 */
    if ((audio_data.fd = open(filename, O_RDONLY)) == -1){
    	fprintf(stderr, "Play - open file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    audio_data.buffer = (char*)calloc(audio_data.exp_frames, audio_data.frame_size); // 分配一个周期长度的缓冲区

    start_time = time(NULL);
    do {
        read_bytes = read(audio_data.fd, audio_data.buffer, audio_data.exp_frames * audio_data.frame_size);
        if (read_bytes == 0) {
            fprintf(stderr, "Play - end of file on input\n");
            break;
        } else if (read_bytes == -1) {
            fprintf(stderr, "Play - read file eoor\n");
            break;
        }

        /* 播放这些数据 */
        audio_data.act_frames = snd_pcm_writei(audio_data.pcm, audio_data.buffer, read_bytes / audio_data.frame_size);
        if (audio_data.act_frames == -EPIPE) {
            /* EPIPE - underrun */
            fprintf(stderr, "Play - underrun occurred\n");
            snd_pcm_prepare(audio_data.pcm);
        } else if (audio_data.act_frames < 0) {
            fprintf(stderr, "Play - error from writei: %s\n", snd_strerror(audio_data.act_frames));
            break;
        }  else if (audio_data.act_frames != audio_data.exp_frames) {
            fprintf(stderr, "Play - short write, write %ld frames\n", audio_data.act_frames);
        }
    } while (stop_flag == 0);

    release_resources(&audio_data);
}

void handle_SIGINT(int sig){
    stop_flag = 1;
}

void set_PCM_hw_params(audio_data_t* audio_data, unsigned int sample_rate, unsigned short bit_depth, unsigned short channels){
    int err = 0;
    unsigned int buffer_time = 1000000;      // 缓冲区长度(us)
    unsigned int period_time = 100000;       // 周期长度(us)
    snd_pcm_hw_params_t* params = NULL; // 硬件参数结构体

    /* 设置alsa设备硬件属性 */
    snd_pcm_hw_params_alloca(&params); // 从栈里分配硬件参数对象内存
	snd_pcm_hw_params_any(audio_data->pcm, params); // 使用默认参数初始化参数对象
	snd_pcm_hw_params_set_rate_near(audio_data->pcm, params, &sample_rate, 0); // 设置采样率
	snd_pcm_hw_params_set_access(audio_data->pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED); // 设置交错模式
    snd_pcm_hw_params_set_format(audio_data->pcm, params, get_alsa_format(bit_depth)); // 设置采样格式(样本长度, 有无符号, 大小端)
	snd_pcm_hw_params_set_channels(audio_data->pcm, params, channels); // 设置通道数
    snd_pcm_hw_params_set_period_size_near(audio_data->pcm, params, &(audio_data->exp_frames), 0); // 设置通道数
    snd_pcm_hw_params_set_buffer_time_near(audio_data->pcm, params, &buffer_time, 0); // 设置缓存大小
    snd_pcm_hw_params_set_period_time_near(audio_data->pcm, params, &period_time, 0); // 设置周期大小

    if ((err = snd_pcm_hw_params(audio_data->pcm, params)) < 0) { // 把上述设置值写入设备
        fprintf(stderr, "Record - write params: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    if ((err = snd_pcm_hw_params_get_period_size(params, &(audio_data->exp_frames), 0)) < 0){ // 获取实际周期大小
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

void release_resources(audio_data_t* audio_data){
    stop_flag = 0;
    snd_pcm_drain(audio_data->pcm);
    snd_pcm_close(audio_data->pcm);
    audio_data->pcm = NULL;
    free(audio_data->buffer);
    audio_data->buffer = NULL;
    if (close(audio_data->fd) == -1) {
        fprintf(stderr, "close file fail\n");
    }
}

char* format_time_interval(int seconds) {
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;

    char* formatted_time = (char*)malloc(9 * sizeof(char));
    if (formatted_time == NULL) {
        return NULL;
    }

    snprintf(formatted_time, 9, "%02d:%02d:%02d", hours, minutes, secs);

    return formatted_time;
}
