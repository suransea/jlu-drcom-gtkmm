## JLU Drcom Gtkmm

Linux only.

#### make

`$ git clone https://github.com/suransea/jlu-drcom-gtkmm.git`

`$ cd jlu-drcom-gtkmm`

`$ mkdir build`

`$ cd build`

`$ cmake ..`

`$ make`

`$ ./jlu-drcom`

#### dependencies

* [boost](https://www.boost.org/): 异步socket, 避免阻塞UI线程

* [spdlog](https://github.com/gabime/spdlog): 记录日志

* [rapidjson](https://github.com/Tencent/rapidjson): 解析配置文件

* [gtkmm](https://www.gtkmm.org/): UI框架

#### others

* 配置文件位置: ~/.config/drcom-client.conf

* 日志位置: ~/drcomclient.log
