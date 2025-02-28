#include <cstdlib>

int main() {
    return system("libcamera-vid -t 0 --width 1920 --height 1080 "
                 "--codec yuv420 --inline | ffmpeg -i - -f v4l2 /dev/video0");
}