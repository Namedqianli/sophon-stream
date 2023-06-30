# sophon-stream encode element

sophon-stream encode element是sophon-stream框架中的一个插件，用于将处理后的图像信息编码为各类视频格式。

## 目录
- [sophon-stream encode element](#sophon-stream-encode-element)
  - [目录](#目录)
  - [1. 特性](#1-特性)
  - [2. 配置参数](#2-配置参数)
  - [3. rtsp使用说明](#3-rtsp使用说明)
  - [4. rtmp使用说明](#4-rtmp使用说明)
  - [5. 输出本地视频文件](#5-输出本地视频文件)
  - [6. 推流服务器](#6-推流服务器)

## 1. 特性
* 支持多种输出格式，如RTSP、RTMP、本地视频文件等。
* 支持多种视频编码格式，如H.264、H.265等。
* 支持多种像素格式，如I420、NV12等。
* 支持多路视频流高性能编码，支持硬件加速。
* 提供灵活的配置选项，如编码器参数、视频流端口、线程数等。
* 可以与其他sophon-stream插件无缝集成，提供编码后的数据流。

## 2. 配置参数
sophon-stream编码器插件具有一些可配置的参数，可以根据需求进行设置。以下是一些常用的参数：

```json
{
  "configure": {
    "encode_type": "RTSP",
    "rtsp_port": "8554",
    "rtmp_port": "1935",
    "enc_fmt": "h264_bm",
    "pix_fmt": "I420"
  },
  "shared_object": "../../../build/lib/libencode.so",
  "device_id": 0,
  "id": 0,
  "name": "encode",
  "side": "sophgo",
  "thread_number": 4
}
```

|      参数名    |    类型    | 默认值 | 说明 |
|:-------------:| :-------: | :------------------:| :------------------------:|
|  encode_type |    字符串     | "RTSP" |编码格式，包括 “RTSP”、“RTMP”、“VIDEO”|
|  rtsp_port   |   字符串    | "8554" | rtsp 端口 |
|  rtmp_port   |   字符串    |  "1935" | rtmp 端口 |
|  enc_fmt    |   字符串    |  "h264_bm"| 编码格式，包括 "h264_bm"，“h265_bm” |
|  pix_fmt    |   字符串    |  "I420"| 像素格式，包括 "I420"，"NV12"|
|  shared_object |   字符串   |  "../../../build/lib/libencode.so"  | libencode 动态库路径 |
|  device_id  |    整数       |  0 | tpu 设备号 |
|     id      |    整数       | 0  | element id |
|     name    |    字符串     | "encode" | element 名称 |
|     side    |    字符串     | "sophgo"| 设备类型 |
| thread_number |    整数     | 4| 启动线程数，需要保证和处理码流数一致 |

> **注意**：
1. 需要保证插件线程数和处理码流数一致
2. encode_type为RTSP时，需保证rtsp_port不为空，encode_type为RTMP时，需保证rtmp_port不为空

## 3. rtsp使用说明
需要本地启动推流服务器，具体用法见[6. 推流服务器](#6-推流服务器)
在`encode.json`中做出以下设置
```json
"encode_type": “RTSP”,
"rtsp_port": "8554"
```

输出视频流URL的格式为：`rtsp://localhost:{rtsp_port}/{channel_id}`

假设rtsp_port为8554，channel_id为0, 此时URL为`rtsp://localhost:8554/0`

## 4. rtmp使用说明
需要本地启动推流服务器，具体用法见[6. 推流服务器](#6-推流服务器)

在`encode.json`中做出以下设置
```json
"encode_type": “RTMP”,
"rtmp_port": "1935"
```

输出视频流URL格式为：`rtmp://localhost:{rtmp_port}/{channel_id}`

假设rtmp_port 为1935，channel_id为0, 此时URL为`rtmp://localhost:1935/0`

## 5. 输出本地视频文件
在`encode.json`中做出以下设置
```json
"encode_type": "VIDEO",
```

输出视频文件名为：`{channel_id}.avi`

假设channel_id为0, 此时文件名为`0.avi`

## 6. 推流服务器
可以使用`rtsp-simple-server`作为推流服务器，启动步骤如下

首先去[官网](https://github.com/bluenviron/mediamtx/releases)下载对应的软件包然后解压

解压缩后打开`rtsp-simple-server.yml`配置文件，修改readTimeout与writeTimeout这两个参数，保存后退出
```yml
# timeout of read operations.
readTimeout: 120s
# timeout of write operations.
writeTimeout: 120s
```

然后启动rtsp-simple-server
```bash
./rtsp-simple-server
```

此时服务器启动成功
```bash
[0/0] rtsp-simple-server v0.0.0
[0/0] [RTSP] UDP/RTP listener opened on :8000
[0/0] [RTSP] UDP/RTCP listener opened on :8001
[0/0] [RTSP] TCP listener opened on :8554
[0/0] [RTMP] listener opened on :1935
[0/0] [HLS] listener opened on :8888
```

rtsp-sample-server配置中rtsp的默认TCP端口是8554，rtmp默认端口是1935，如果修改端口号，插件配置中相应端口配置也要修改成一致。

github：https://github.com/bluenviron/mediamtx

需要注意的是，rtsp-sample-server是一个示例服务器，并不具备高度的可扩展性和功能完整性。如果您需要构建一个稳定和功能丰富的实际RTSP流媒体服务器，可以选择使用成熟的开源或商业解决方案，如Live555、GStreamer、FFmpeg等，这些工具提供了更广泛和全面的RTSP功能支持。

