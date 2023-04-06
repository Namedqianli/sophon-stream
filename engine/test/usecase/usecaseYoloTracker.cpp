#include "gtest/gtest.h"
#include "common/Logger.h"
#include "framework/Engine.h"
#include "element/MandatoryLink.h"
#include "common/ErrorCode.h"
#include "common/ObjectMetadata.h"
#include "common/type_trans.hpp"
#include "config.h"
#include <fstream>

#include <opencv2/opencv.hpp>


#define DECODE_ID 7000
#define YOLO_ID 7001
#define TRACK_ID 7002
#define ENCODE_ID 7003
#define REPORT_ID 7777


TEST(TestMultiAlgorithmGraph, MultiAlgorithmGraph)
{
    std::string coco_file = "../test/coco.names";
    std::vector<std::string> coco_classnames;
    std::ifstream ifs(coco_file);
    if (ifs.is_open()) {
        std::string line;
        while(std::getline(ifs, line)) {
                line = line.substr(0, line.length() - 1);
                coco_classnames.push_back(line);
        }
    }
    auto & engine = sophon_stream::framework::SingletonEngine::getInstance();

    nlohmann::json graphConfigure;
    graphConfigure["graph_id"] = 1;
    nlohmann::json ElementsConfigure;
    ElementsConfigure.push_back(makeDecoderElementConfig(DECODE_ID, "decoder_element", "sophgo", 0, 1, 0, false, 1, "../lib/libmultiMediaApi.so"));
    ElementsConfigure.push_back(makeElementConfig(REPORT_ID,"report_element","host",0,1,0,false,1, {}));
    
    nlohmann::json yolov5Json = makeAlgorithmConfig("../lib/libalgorithmApi.so","cocoDetect","Yolov5",
        { "../models/yolov5.bmodel" },
        1, { "input.1" }, { 1 }, {{3, 640, 640}},  {"147","148","149"},
        {3}, {{3,80,80,85},{3,40,40,85},{3,20,20,85}},
        { 0.5,0.5 },coco_classnames.size(),coco_classnames);
    
    ElementsConfigure.push_back(makeElementConfig(YOLO_ID, "action_element", "sophgo", 0, 1, 200, false, 1, {yolov5Json}));
    nlohmann::json encodeJson = makeEncodeConfig("../lib/libalgorithmApi.so","","encode_picture",1);
    ElementsConfigure.push_back(makeElementConfig(ENCODE_ID,"action_element","host",0,1,200,true,1, {encodeJson}));

    nlohmann::json trackJson = makeTrackerConfig("../lib/libalgorithmApi.so","trackerSort","tracker_sort",
            1, 1, 0.25f, 20, 3, 4, 3000000,400.0f, 0.7f, 1.3f, 640.0f,360.0f, 10.0f, 19.0f, 112.0f );
    ElementsConfigure.push_back(makeElementConfig(TRACK_ID, "tracker_element", "sophgo", 0, 1, 200, false, 1, {trackJson}));
    
    graphConfigure["elements"] = ElementsConfigure;
    
    graphConfigure["connections"].push_back(makeConnectConfig(DECODE_ID,0,YOLO_ID,0));
    graphConfigure["connections"].push_back(makeConnectConfig(YOLO_ID,0,ENCODE_ID,0));
    graphConfigure["connections"].push_back(makeConnectConfig(YOLO_ID,0,TRACK_ID,0));
    graphConfigure["connections"].push_back(makeConnectConfig(ENCODE_ID,0,REPORT_ID,0));

    std::mutex mtx;
    std::condition_variable cv;

    engine.addGraph(graphConfigure.dump());
    engine.setDataHandler(1,REPORT_ID,0,[&](std::shared_ptr<void> data) {
        auto objectMetadata = std::static_pointer_cast<sophon_stream::common::ObjectMetadata>(data);
        if(objectMetadata == nullptr)
            return;
        cv::Mat cpuMat;
        if(objectMetadata->mPacket && objectMetadata->mPacket->mData)
        {
            std::vector<uchar> inputarray;
            uchar * p = static_cast<uchar*>(objectMetadata->mPacket->mData.get());
            for(int i=0;i<objectMetadata->mPacket->mDataSize;i++){
                inputarray.push_back(p[i]);
            }
            cpuMat = cv::imdecode(inputarray, 1);
            for(int i=0;i<objectMetadata->mSubObjectMetadatas.size();i++){
                auto detectData = objectMetadata->mSubObjectMetadatas[i]->mSpDataInformation;
                cv::rectangle(cpuMat, cv::Rect(detectData->mBox.mX, detectData->mBox.mY, detectData->mBox.mWidth, detectData->mBox.mHeight), 
                        cv::Scalar(0, 255, 0), 2);

                for(auto pair:detectData->mKeyPoints){
                    cv::circle(cpuMat, cv::Point(pair.second.mPoint.mX, pair.second.mPoint.mY), 1, cv::Scalar(255, 255, 0), 1, 8, 0);
                }
                cv::putText(cpuMat, std::to_string(objectMetadata->mSubObjectMetadatas[i]->mTrackedObjectMetadata->mTrackId), cv::Point(detectData->mBox.mX, detectData->mBox.mY), cv::HersheyFonts::FONT_HERSHEY_PLAIN, 
                        1.5, cv::Scalar(255, 255, 0), 2);
                
                cv::putText(cpuMat, std::to_string(objectMetadata->mSubObjectMetadatas[i]->getTimestamp()), cv::Point(detectData->mBox.mX+detectData->mBox.mWidth, detectData->mBox.mY+detectData->mBox.mHeight),
                        cv::HersheyFonts::FONT_HERSHEY_PLAIN, 
                        1.5, cv::Scalar(255, 255, 0), 2);

                int subSubDataLen = objectMetadata->mSubObjectMetadatas[i]->mSubObjectMetadatas.size();
                for(int j=0;j<subSubDataLen;j++){
                    auto subSubData = objectMetadata->mSubObjectMetadatas[i]->mSubObjectMetadatas[j];
                }
            }
            cv::imwrite("out.jpg", cpuMat);
            if(objectMetadata->mPacket->mEndOfStream) cv.notify_one();
        }
    });
    nlohmann::json decodeConfigure;
    decodeConfigure["channel_id"] = 1;
    decodeConfigure["url"] = "../test/out.avi";
    decodeConfigure["resize_rate"] = 2.0f;
    decodeConfigure["timeout"] = 0;
    decodeConfigure["multimedia_name"] = "decode_picture";
    decodeConfigure["source_type"] = 0;
    decodeConfigure["reopen_times"] = -1;
        
    auto channelTask = std::make_shared<sophon_stream::element::ChannelTask>();
    channelTask->request.operation = sophon_stream::element::ChannelOperateRequest::ChannelOperate::START;
    channelTask->request.json = decodeConfigure.dump();
    sophon_stream::common::ErrorCode errorCode = engine.sendData(1,
                                DECODE_ID,
                                0,
                                std::static_pointer_cast<void>(channelTask),
                                std::chrono::milliseconds(200));
    {
        std::unique_lock<std::mutex> uq(mtx);
        cv.wait(uq);
    }
}