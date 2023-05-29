//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-STREAM is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#include "Yolov5SophgoContext.h"

#include <nlohmann/json.hpp>

#define JSON_ALGORITHM_NAME_FIELD "algorithm_name"
#define JSON_ALGORITHM_MODEL_PATH_FIELD "model_path"
#define JSON_ALGORITHM_MAX_BATCHSIZE_FIELD "max_batchsize"
#define JSON_ALGORITHM_INPUT_NODE_NAME_FIELD "input_node_name"
#define JSON_ALGORITHM_INPUT_SHAPE_FIELD "input_shape"
#define JSON_ALGORITHM_NUM_INPUTS_FIELD "num_inputs"
#define JSON_ALGORITHM_OUTPUT_NODE_NAME_FIELD "output_node_name"
#define JSON_ALGORITHM_OUTPUT_SHAPE_FIELD "output_shape"
#define JSON_ALGORITHM_THRETHOLD_FIELD "threthold"
#define JSON_ALGORITHM_NUM_CLASS_FIELD "num_class"
#define JSON_ALGORITHM_NUM_OUTPUTS_FIELD "num_outputs"
#define JSON_ALGORITHM_MIN_SIZE_FIELD "min_size"
#define JSON_ALGORITHM_PYRAMID_FIELD "pyramid"
#define JSON_ALGORITHM_LABEL_NAMES_FIELD "label_names"
#define JSON_ALGORITHM_USE_TPU_KERNEL "use_tpu_kernel"

namespace sophon_stream {
namespace element {
namespace yolov5 {

float get_aspect_scaled_ratio(int src_w, int src_h, int dst_w, int dst_h,
                              bool* pIsAligWidth) {
  float ratio;
  float r_w = (float)dst_w / src_w;
  float r_h = (float)dst_h / src_h;
  if (r_h > r_w) {
    *pIsAligWidth = true;
    ratio = r_w;
  } else {
    *pIsAligWidth = false;
    ratio = r_h;
  }
  return ratio;
}

Yolov5SophgoContext::~Yolov5SophgoContext() {
  // std::cout << "sophgo context uninit" << std::endl;

  // int input_num = m_bmNetwork->outputTensorNum();
  // for(int i = 0;i < input_num; ++i)
  // {
  //         bm_free_device(m_bmContext->handle(), out_dev_mem[i]);
  //         bm_free_device(m_bmContext->handle(), detect_num_mem[i]);
  // }
}

/**
 * context初始化
 * @param[in] json: 初始化的json字符串
 * @return 错误码
 */
common::ErrorCode Yolov5SophgoContext::init(const std::string& json) {
  common::ErrorCode errorCode = common::ErrorCode::SUCCESS;
  do {
    auto configure = nlohmann::json::parse(json, nullptr, false);
    if (!configure.is_object()) {
      errorCode = common::ErrorCode::PARSE_CONFIGURE_FAIL;
      break;
    }

    auto algotirhmNameIt = configure.find(JSON_ALGORITHM_NAME_FIELD);
    if (configure.end() == algotirhmNameIt || !algotirhmNameIt->is_string()) {
      errorCode = common::ErrorCode::PARSE_CONFIGURE_FAIL;
      break;
    }

    auto tpu_kernelIt = configure.find(JSON_ALGORITHM_USE_TPU_KERNEL);
    if (configure.end() == tpu_kernelIt || !tpu_kernelIt->is_boolean()) {
      errorCode = common::ErrorCode::PARSE_CONFIGURE_FAIL;
      break;
    }
    use_tpu_kernel = tpu_kernelIt->get<bool>();

    auto maxBatchSizeCon = configure.find(JSON_ALGORITHM_MAX_BATCHSIZE_FIELD);
    if (configure.end() != maxBatchSizeCon &&
        maxBatchSizeCon->is_number_integer()) {
      maxBatchSize = maxBatchSizeCon->get<int>();
    }

    auto numClassCon = configure.find(JSON_ALGORITHM_NUM_CLASS_FIELD);
    if (configure.end() != numClassCon && numClassCon->is_number_integer()) {
      numClass = numClassCon->get<int>();
    }

    auto modelPathCon = configure.find(JSON_ALGORITHM_MODEL_PATH_FIELD);
    for (auto& temp : *modelPathCon) {
      //            if (configure.end() != temp
      //                    && temp.is_string()) {
      modelPath.push_back(temp.get<std::string>());
      //            }
    }

    auto inputNodeCon = configure.find(JSON_ALGORITHM_INPUT_NODE_NAME_FIELD);
    for (auto& temp : *inputNodeCon) {
      //            if (configure.end() != temp
      //                    && temp.is_string()) {
      inputNodeName.push_back(temp.get<std::string>());
      //            }
    }

    auto inputShapeCon = configure.find(JSON_ALGORITHM_INPUT_SHAPE_FIELD);
    for (auto& temp : *inputShapeCon) {
      //            if (configure.end() != temp
      //                    && temp.is_array()) {
      nodeDims node;
      node.c = temp[0].get<int>();
      node.h = temp[1].get<int>();
      node.w = temp[2].get<int>();
      inputShape.push_back(node);
      //            }
    }

    auto numInputsCon = configure.find(JSON_ALGORITHM_NUM_INPUTS_FIELD);
    for (auto& temp : *numInputsCon) {
      //            if (configure.end() != temp
      //                    && temp.is_number_integer()) {
      numInputs.push_back(temp.get<int>());
      //            }
    }

    auto outputNodeCon = configure.find(JSON_ALGORITHM_OUTPUT_NODE_NAME_FIELD);
    for (auto& temp : *outputNodeCon) {
      //            if (configure.end() != temp
      //                    && temp.is_string()) {
      outputNodeName.push_back(temp.get<std::string>());
      //            }
    }

    auto outputShapeCon = configure.find(JSON_ALGORITHM_OUTPUT_SHAPE_FIELD);
    for (auto& temp : *outputShapeCon) {
      //            if (configure.end() != temp
      //                    && temp.is_array()) {
      nodeDims node;
      node.c = temp[0].get<int>();
      node.h = temp[1].get<int>();
      node.w = temp[2].get<int>();
      outputShape.push_back(node);
      //            }
    }

    auto numOutputsCon = configure.find(JSON_ALGORITHM_NUM_OUTPUTS_FIELD);
    for (auto& temp : *numOutputsCon) {
      //            if (configure.end() != temp
      //                    && temp.is_number_integer()) {
      numOutputs.push_back(temp.get<int>());
      //            }
    }

    auto thretholdCon = configure.find(JSON_ALGORITHM_THRETHOLD_FIELD);
    for (auto& temp : *thretholdCon) {
      //            if (configure.end() != temp
      //                    && temp.is_number_float()) {
      threthold.push_back(temp.get<float>());
      //            }
    }
    // TODO: parse other configure field
    auto minSizeCon = configure.find(JSON_ALGORITHM_MIN_SIZE_FIELD);
    if (configure.end() != minSizeCon && minSizeCon->is_number_integer()) {
      // minSize = minSizeCon->get<int>();
    }

    auto labelNamesCon = configure.find(JSON_ALGORITHM_LABEL_NAMES_FIELD);
    for (auto& temp : *labelNamesCon) {
      //            if (configure.end() != temp
      //                    && temp.is_string()) {
      labelNames.push_back(temp.get<std::string>());
      //            }
    }

    auto pyramidCon = configure.find(JSON_ALGORITHM_PYRAMID_FIELD);
    if (configure.end() != pyramidCon && pyramidCon->is_number_integer()) {
      // pyramid = pyramidCon->get<int>();
    }

    // TODO: parse other configure field

  } while (false);

  return errorCode;
}
}  // namespace yolov5
}  // namespace element
}  // namespace sophon_stream