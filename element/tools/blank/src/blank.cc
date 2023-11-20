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

common::ErrorCode Blank::doWork(int dataPipeId) {
  std::vector<int> inputPorts = getInputPorts();
  int outputPort = 0;
  if (!getSinkElementFlag()) {
    std::vector<int> outputPorts = getOutputPorts();
    outputPort = outputPorts[0];
  }

  common::ObjectMetadatas inputs;

  for (auto inputPort : inputPorts) {
    auto data = popInputData(inputPort, dataPipeId);
    while (!data && (getThreadStatus() == ThreadStatus::RUN)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      data = popInputData(inputPort, dataPipeId);
    }
    if (data == nullptr) return common::ErrorCode::SUCCESS;

    auto objectMetadata =
        std::static_pointer_cast<common::ObjectMetadata>(data);
    inputs.emplace_back(objectMetadata);
    IVS_INFO("Got Input, port id = {0}, channel_id = {1}, frame_id = {2}",
             inputPort, objectMetadata->mFrame->mChannelId,
             objectMetadata->mFrame->mFrameId);
  }

  for (auto objectMetadata : inputs) {
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
  }

  return common::ErrorCode::SUCCESS;
}

REGISTER_WORKER("blank", Blank)
}  // namespace blank
}  // namespace element
}  // namespace sophon_stream