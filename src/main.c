#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char**argv){
    int play = 0;
    int record = 0;
    const char *filename = NULL;
    int sample_rate = 8000;     // 默认采样率
    int bit_depth = 16;         // 默认位深
    int channels = 2;           // 默认声道数

    for(int i = 1; i < argc; ++i) {
        // 打印帮助信息
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--play") == 0) {
            play = 1;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--record") == 0) {
            record = 1;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--samplerate") == 0) {
            if (i + 1 < argc) {
                sample_rate = atoi(argv[++i]);
            } else {
                fprintf(stderr, "Error: Missing sample rate after %s\n", argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--bitdepth") == 0) {
            if (i + 1 < argc) {
                bit_depth = atoi(argv[++i]);
            } else {
                fprintf(stderr, "Error: Missing bit depth after %s\n", argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--channels") == 0) {
            if (i + 1 < argc) {
                channels = atoi(argv[++i]);
            } else {
                fprintf(stderr, "Error: Missing channels after %s\n", argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0){
            if (i + 1 < argc) {
                filename = argv[++i];
            } else {
                fprintf(stderr, "Error: Missing filename after %s\n", argv[i]);
                return 1;
            }
        } else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("welcome to use %s:)\n", _PROJECT_NAME);
            printf("Usage: %s [options] <arguments>\n", _PROJECT_NAME);
            printf("\nOptions:\n");
            printf("  -r, --record                  Record audio to a file\n");
            printf("    -s, --samplerate <rate>     Set the sample rate (e.g., 8000, 44100, 96000)\n");
            printf("    -b, --bitdepth <depth>      Set the bit depth (e.g., 16, 24, 32)\n");
            printf("    -c, --channels <num>        Set the number of channels (e.g., 1 for mono, 2 for stereo)\n");
            printf("  -p, --play                    Play audio from a file\n");
            printf("  -f, --file <filename>         Specify the audio file path\n");
            printf("  -h, --help                    Display this help message and exit\n");
            printf("  -v, --version                 Display version and exit\n");
            printf("\nExamples:\n");
            printf("  Record audio with sample rate 44100 Hz, bit depth 16, and 2 channels:\n");
            printf("    %s --record --file output.wav --samplerate 44100 --bitdepth 16 --channels 2\n", _PROJECT_NAME);
            printf("\n  Play audio from a file:\n");
            printf("    %s --play --file output.wav\n", _PROJECT_NAME);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("%s\t%s\n", _PROJECT_NAME, _PROJECT_VERSION);
        } else {
            printf("unknown option: %s\n", argv[i]);
            printf("Usage: %s [-r, --record] [-s, --samplerate <rate>] [-b, --bitdepth <depth>] [-c, --channels <num>] \n", _PROJECT_NAME);
            printf("\t\t[-p, --play] [-f, --file <filename>] [-h, --help] [-v, --version] \n");
        }
    }
    return 0;
}