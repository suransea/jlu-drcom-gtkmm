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

* boost: 异步socket, 避免阻塞UI线程 [https://www.boost.org/]

* spdlog: 记录日志 [https://github.com/gabime/spdlog]

* rapidjson: 解析配置文件 [https://github.com/Tencent/rapidjson]

* gtkmm: UI框架 [https://www.gtkmm.org/]

* gtk+: gtkmm所需 [https://www.gtk.org/]

#### others

* 配置文件位置: ~/.config/drcom-client.conf

* 日志位置: ~/drcomclient.log
