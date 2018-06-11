example

1. get time

`request`: （冒号后带一个空格，如果内容要求为空，则不带空格）

```c
packet-type: 1
request-content:
```

`response`：

```c++
packet-type: 1
response-content: Sat Jun  9 22:46:08 2018
```

2. get name

`request`: （冒号后带一个空格，如果内容要求为空，则不带空格）

```c
packet-type: 2
request-content:
```

`response`：

```c++
packet-type: 2
response-content: 椎名ましろ
```

3. get list

`request`: （冒号后带一个空格，如果内容要求为空，则不带空格）

```c
packet-type: 3
request-content:
```

`response`：content中一行数据的顺序为index IP port

```c++
packet-type: 3
response-content: 0 10.0.0.1 1
1 10.0.0.2 2

```

4. send message

`request`: （冒号后带一个空格，如果内容要求为空，则不带空格）

```c
packet-type: 4-Send
request-content: request-index: 1
message: Hello World!
```

`request`: （A通过服务器给B发消息，B成功收到消息回应给服务器的消息）(from-index指的是a的index)

```c
packet-type: 4-Receive
request-content: from-index: 0
message: Hello World!
```

`response`：（成功）

```c++
packet-type: 4-S
response-content: request-index: 1
message: Hello World!
```

`response`：（编号不存在，可能是输入错误或对方已断线）

```c++
packet-type: 4-F-not-exist
response-content:
```

`response`：（转发的消息）

```c++
packet-type: message
response-content: from: 1
Hello World!
```

5. ERROR

`request`: （冒号后带一个空格，如果内容要求为空，则不带空格）错误的包，type种类不对

```c
packet-type: dwadf
request-content: adafsda
```

`response`：

```c++
packet-type: 5
response-content:
```







