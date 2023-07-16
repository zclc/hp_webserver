# hp_webserver

## 项目简介
一款轻量级，高性能的httpserver

**关键特性：**
* 有限状态机解析http请求
* 线程池
* 内存池
* 堆定时器
* zlog日志

## 构建运行
可以修改conf/main.conf 中端口的设置，默认8888端口
1. `cmake -B build`
2. `cmake --build build`
3. `./build/main -c conf/main.conf`

## 运行演示
![演示gif](/html/webserver.gif)

## 项目结构

![websever整体结构](/html/websever.png)
