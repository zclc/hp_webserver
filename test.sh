#!/bin/bash

# 创建一个空数组来存储结果
results=()

# 使用循环运行指令10次
for ((i=1; i<=10; i++))
do
  # 运行指令，并将结果存储在变量中
  result=$(webbench -c 1000 -t 5 http://47.100.86.209:8888/ | grep -oP 'Speed.*?(?=,)')

  # 将结果添加到数组中
  results+=("$result")
done

# 打印结果
for result in "${results[@]}"
do
  echo "$result"
done