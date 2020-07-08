# Esp8266-oled-timeclock
基于ESP8266和SSD1306 128x64的时钟显示

其中`oled_OLED_esp8266`下面的`MyRealTimeClock`为第三方库，Arduino中的库管理器没有，所以就上传上来了

下载后将此文件夹放到`C:\Users\用户名称\Documents\Arduino\libraries`文件夹中，

> 或者放到Arduino安装目录下的`librariesl`文件夹中（不推荐）

其中`MyRealTimeClock`是DS2312的驱动库，本实例没有使用此库，原来打算用DS2312的。

文件中包含了DS2312的函数声明，但是没有使用。纯ntp实现