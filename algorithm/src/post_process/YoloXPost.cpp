#include "YoloXPost.h"
#include "../context/SophgoContext.h"
#include "common/Logger.h"
#include "common/type_trans.hpp"
#include "common/Clocker.h"
#include "cmath"
#include "algorithm"

namespace sophon_stream {
namespace algorithm {
namespace post_process {

void YoloXPost::init(algorithm::Context& context)
{

}

int YoloXPost::argmax(float* data, int num)
{
    float max_value = 0.0;
    int max_index = 0;
    for(int i = 0; i < num; ++i) {
    float value = data[i];
    if (value > max_value) {
        max_value = value;
        max_index = i;
        }
    }
    return max_index;
}

float YoloXPost::sigmoid(float x){
  return 1.0 / (1 + expf(-x));
}

// void YoloXPost::NMS(YoloXBoxVec &dets, float nmsConfidence)
// {
//     int length = dets.size();
//     int index = length - 1;

//     std::sort(dets.begin(), dets.end(), [](const YoloXBox& a, const YoloXBox& b) {
//         return a.score < b.score;
//         });

//     std::vector<float> areas(length);
//     for (int i=0; i<length; i++)
//     {
//         areas[i] = dets[i].width * dets[i].height;
//     }

//     while (index  > 0)
//     {
//         int i = 0;
//         while (i < index)
//         {
//             float left    = std::max(dets[index].x,   dets[i].x);
//             float top     = std::max(dets[index].y,    dets[i].y);
//             float right   = std::min(dets[index].x + dets[index].width,  dets[i].x + dets[i].width);
//             float bottom  = std::min(dets[index].y + dets[index].height, dets[i].y + dets[i].height);
//             float overlap = std::max(0.0f, right - left) * std::max(0.0f, bottom - top);
//             if (overlap / (areas[index] + areas[i] - overlap) > nmsConfidence)
//             {
//                 areas.erase(areas.begin() + i);
//                 dets.erase(dets.begin() + i);
//                 index --;
//             }
//             else
//             {
//                 i++;
//             }
//         }
//         index--;
//     }
// }

float overlap_FM(float x1, float w1, float x2, float w2)
{
	float l1 = x1;
	float l2 = x2;
	float left = l1 > l2 ? l1 : l2;
	float r1 = x1 + w1;
	float r2 = x2 + w2;
	float right = r1 < r2 ? r1 : r2;
	return right - left;
}

float box_intersection_FM(YoloXBox a, YoloXBox b)
{
	float w = overlap_FM(a.left, a.width, b.left, b.width);
	float h = overlap_FM(a.top, a.height, b.top, b.height);
	if (w < 0 || h < 0) return 0;
	float area = w*h;
	return area;
}

float box_union_FM(YoloXBox a, YoloXBox b)
{
	float i = box_intersection_FM(a, b);
	float u = a.width*a.height + b.width*b.height - i;
	return u;
}

float box_iou_FM(YoloXBox a, YoloXBox b)
{
	return box_intersection_FM(a, b) / box_union_FM(a, b);
}

static void nms_sorted_bboxes(const std::vector<YoloXBox>& objects, std::vector<int>& picked, float nms_threshold)
{
    picked.clear();
    const int n = objects.size();

    for (int i = 0; i < n; i++)    {
        const YoloXBox& a = objects[i];
        int keep = 1;
        for (int j = 0; j < (int)picked.size(); j++)
        {
            const YoloXBox& b = objects[picked[j]];

            float iou = box_iou_FM(a, b);
            if (iou > nms_threshold)
                keep = 0;
        }
        if (keep)
            picked.push_back(i);
    }
}


void YoloXPost::postProcess(algorithm::Context& context, common::ObjectMetadatas& objectMetadatas)
{
    Clocker clocker;
    // std::vector<YoloBoxVec> detections;
    context::SophgoContext* pSophgoContext = dynamic_cast<context::SophgoContext*>(&context);
    std::vector<std::shared_ptr<BMNNTensor>> outputTensors(pSophgoContext->output_num);
    // 三个输出，[batch,box_num,4],[,,1],[,,class_num]
    for(int i=0; i<pSophgoContext->output_num; i++){
        outputTensors[i] = pSophgoContext->m_bmNetwork->outputTensor(i);
    }
    int frame_width = pSophgoContext->m_frame_w;
    int frame_height = pSophgoContext->m_frame_h;
    int net_w = pSophgoContext->m_net_w;
    int net_h = pSophgoContext->m_net_h;


    // 这部分放到外面
    int outlen_dim = 0;
    std::vector<int> strides {8,16,32};
    for(int i = 0;i<strides.size();++i)
    {
        int layer_w = net_w / strides[i];
        int layer_h = net_h / strides[i];
        outlen_dim += layer_w * layer_h; // 8400
    }
    int * grids_x_ = new int [outlen_dim];
    int * grids_y_ = new int [outlen_dim];
    int * expanded_strides_ = new int [outlen_dim];
    int channel_len = 0;
    for (int i=0;i<strides.size();++i)  {
        int layer_w = net_w/strides[i];
        int layer_h = net_h/strides[i];
        for (int m = 0; m < layer_h; ++m)   {
            for (int n = 0; n < layer_w; ++n)    {
                grids_x_[channel_len+m*layer_w+n] = n;
                grids_y_[channel_len+m*layer_w+n] = m;
                expanded_strides_[channel_len+m*layer_w+n] = strides[i];
            }
        }
        channel_len += layer_w * layer_h;
    }
    // end notes

    for(int batch_idx = 0; batch_idx < pSophgoContext->max_batch; ++batch_idx)
    {

        if(pSophgoContext->mEndOfStream) continue;
        
        int tx1 = 0, ty1 = 0;

        float scale_w = float(net_w) / frame_width;
        float scale_h = float(net_h) / frame_height;
        float scale_min = scale_h < scale_w ? scale_h : scale_w;

        float scale_x = 1.0 / scale_min;
        float scale_y = 1.0 / scale_min;

        int objectOffset = batch_idx * outlen_dim * 1;
        int boxOffset = batch_idx * outlen_dim * 4;
        int classOffset = batch_idx * outlen_dim * pSophgoContext->m_class_num;
        
        YoloXBoxVec yolobox_vec;

        for(size_t i = 0; i < outlen_dim; ++i)
        {
            // 取出物体置信度
            // auto output_tensor = outputTensors[tidx];
            float* objectTensor = (float*)outputTensors[1]->get_cpu_data();
            float box_objectness = objectTensor[objectOffset + i];
            if(box_objectness < pSophgoContext->m_thresh[0])
                continue;
            // 进入解码阶段
            float* boxTensor = (float*)outputTensors[0]->get_cpu_data();
            float center_x = (boxTensor[boxOffset + i * 4 + 0] + grids_x_[i]) * expanded_strides_[i];
            float center_y = (boxTensor[boxOffset + i * 4 + 1] + grids_y_[i]) * expanded_strides_[i];
            float w_temp = exp(boxTensor[boxOffset + i * 4 + 2]) * expanded_strides_[i];
            float h_temp = exp(boxTensor[boxOffset + i * 4 + 3]) * expanded_strides_[i];
            center_x *= scale_x;
            center_y *= scale_y;
            w_temp *= scale_x;
            h_temp *= scale_y;
            float left = center_x - w_temp/2;
            float top = center_y - h_temp/2;
            float right = center_x + w_temp/2;
            float bottom = center_y + h_temp/2;

            for(int class_idx = 0;class_idx < pSophgoContext->m_class_num;++class_idx)
            {
                float* classTensor = (float*)outputTensors[2]->get_cpu_data();
                float box_cls_score = classTensor[classOffset + i * pSophgoContext->m_class_num + class_idx];
                float box_prob = box_objectness * box_cls_score;
                if(box_prob > pSophgoContext->m_thresh[0])
                {
                    YoloXBox box;
                    box.width = w_temp;
                    box.height = h_temp;
                    box.left = left;
                    box.top = top;
                    box.right = right;
                    box.bottom = bottom;
                    box.score = box_prob;
                    box.class_id = class_idx;
                    yolobox_vec.push_back(box);
                }
            }
        }
        std::sort(yolobox_vec.begin(), yolobox_vec.end(),[](const YoloXBox & a, const YoloXBox & b){
            return a.score > b.score;
        });

        std::vector<YoloXBox> dect_temp_batch;
        std::vector<int> picked;
        nms_sorted_bboxes(yolobox_vec, picked, pSophgoContext->m_thresh[1]);
        for (size_t i = 0; i < picked.size(); i++)    {
            dect_temp_batch.push_back(yolobox_vec[picked[i]]);
        }
        // detections.push_back(dect_temp_batch);
        for (auto bbox : dect_temp_batch) {
            std::shared_ptr<common::ObjectMetadata> spObjData = std::make_shared<common::ObjectMetadata>();
            spObjData->mDetectedObjectMetadata = std::make_shared<common::DetectedObjectMetadata>();
            spObjData->mDetectedObjectMetadata->mBox.mX = bbox.left;
            spObjData->mDetectedObjectMetadata->mBox.mY = bbox.top;
            spObjData->mDetectedObjectMetadata->mBox.mWidth = bbox.width;
            spObjData->mDetectedObjectMetadata->mBox.mHeight = bbox.height;
            spObjData->mDetectedObjectMetadata->mScores.push_back(bbox.score);
            spObjData->mDetectedObjectMetadata->mClassify = bbox.class_id;
            objectMetadatas[batch_idx]->mSubObjectMetadatas.push_back(spObjData);
        }
    }
    std::cout<<"yoloX post cost: "<<clocker.tell_us()<<std::endl;
    
}



}
}
}