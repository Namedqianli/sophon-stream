1.yolox目标检测算法插件
为了提高pipeline性能，模型前处理、推理、后处理在3个element；通过设置`stage`字段指明element具体是前处理、推理、后处理

2.配置
```
{
    "configure":{
        "model_path":"../data/models/BM1684X/yolox_s_int8_4b.bmodel",
        "threshold_conf":0.5,
        "threshold_nms":0.5,
        "stage":["infer"]
    },
    "shared_object":"../../../element/algorithm/yolox/build/libyolox.so",
    "device_id":0,
    "id":0,
    "name":"yolox",
    "side":"sophgo",
    "thread_number":1
}
```