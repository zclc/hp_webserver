# hp_webserver

## 项目简介
一款轻量级，高性能的httpserver

**关键特性：**
* 实现http get，post和长连接请求
* 无锁队列的线程池
* 内存池
* zlog日志
* 堆定时器

# 构建运行
可以修改zaver.conf 中端口的设置，默认8888端口
1. `cmake -B build`
2. `cmake --build build`
3. `./build/main -c conf/main.conf`

## 项目演示


## 性能测试
webech测试结果

