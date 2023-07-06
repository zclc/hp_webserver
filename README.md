# hp_webserver

## 项目简介
一款轻量级，高性能的httpserver

**特点：**
* 实现get和长连接请求
* 实现了一个线程池
* 实现了http的内存池
* 使用了zlog日志
* 实现了基于二叉堆的定时器

# 构建运行
可以修改zaver.conf 中端口的设置，默认8888端口
`cmake -B build`
`cmake --build build`
`./build/zaver -c zaver.conf`

## 项目演示


## 性能测试
webech测试结果

