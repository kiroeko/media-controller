#include "app/media_controller_app.h"

// 固件入口只创建应用，由应用负责板级初始化与持续运行。
int main() {
    MediaControllerApp app;
    app.run();
}
