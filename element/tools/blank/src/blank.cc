//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-STREAM is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#include "blank.h"

#include "common/logger.h"
#include "element_factory.h"

namespace sophon_stream {
namespace element {
namespace blank {
Blank::Blank() {}
Blank::~Blank() {}

common::ErrorCode Blank::initInternal(const std::string& json) {
  return common::ErrorCode::SUCCESS;
}
void Blank::uninitInternal() {}

common::ErrorCode Blank::doWork(int dataPipeId) {
  std::vector<int> inputPorts = getInputPorts();
  int inputPort = inputPorts[0];
  int outputPort = 0;
  if (!getSinkElementFlag()) {
    std::vector<int> outputPorts = getOutputPorts();
    int outputPort = outputPorts[0];
  }

  auto data = popInputData(inputPort, dataPipeId);
  while (!data && (getThreadStatus() == ThreadStatus::RUN)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    data = popInputData(inputPort, dataPipeId);
  }
  if (data == nullptr) return common::ErrorCode::SUCCESS;

  auto objectMetadata = std::static_pointer_cast<common::ObjectMetadata>(data);

  // if (objectMetadata->mFrame != nullptr &&
  //     objectMetadata->mFrame->mSpData != nullptr) {
  //   std::string filename =
  //       std::to_string(objectMetadata->mFrame->mChannelId) + "-" +
  //       std::to_string(objectMetadata->mFrame->mFrameId) + "-" +
  //       std::to_string(subId) + ".bmp";
  //   bm_image_write_to_bmp(*objectMetadata->mFrame->mSpData, filename.c_str());
  //   ++ subId;
  // }

  // usleep(10);

  int channel_id_internal = objectMetadata->mFrame->mChannelIdInternal;
  int outDataPipeId =
      getSinkElementFlag()
          ? 0
          : (channel_id_internal % getOutputConnectorCapacity(outputPort));
  common::ErrorCode errorCode =
      pushOutputData(outputPort, outDataPipeId,
                     std::static_pointer_cast<void>(objectMetadata));
  if (common::ErrorCode::SUCCESS != errorCode) {
    IVS_WARN(
        "Send data fail, element id: {0:d}, output port: {1:d}, data: "
        "{2:p}",
        getId(), outputPort, static_cast<void*>(objectMetadata.get()));
  }
  return common::ErrorCode::SUCCESS;
}

REGISTER_WORKER("blank", Blank)
}  // namespace blank
}  // namespace element
}  // namespace sophon_stream