//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-STREAM is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#include "osd.h"

#include <stdlib.h>

#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "common/logger.h"
#include "draw_utils.h"
#include "element_factory.h"

namespace sophon_stream {
namespace element {
namespace osd {
Osd::Osd() {}

Osd::~Osd() {}

common::ErrorCode Osd::initInternal(const std::string& json) {
  common::ErrorCode errorCode = common::ErrorCode::SUCCESS;
  do {
    // json是否正确
    auto configure = nlohmann::json::parse(json, nullptr, false);
    if (!configure.is_object()) {
      errorCode = common::ErrorCode::PARSE_CONFIGURE_FAIL;
      break;
    }
    mFpsProfiler.config("fps_osd", 100);
    std::string osd_type =
        configure.find(CONFIG_INTERNAL_OSD_TYPE_FIELD)->get<std::string>();
    mOsdType = OsdType::TRACK;
    if (osd_type == "DET") mOsdType = OsdType::DET;
    if (osd_type == "TRACK") mOsdType = OsdType::TRACK;

    if (mOsdType == OsdType::DET) {
      std::string class_names_file =
          configure.find(CONFIG_INTERNAL_CLASS_NAMES_FIELD)->get<std::string>();
      std::ifstream istream;
      istream.open(class_names_file);
      assert(istream.is_open());
      std::string line;
      while (std::getline(istream, line)) {
        line = line.substr(0, line.length() - 1);
        mClassNames.push_back(line);
      }
      istream.close();
    }

  } while (false);
  return errorCode;
}

common::ErrorCode Osd::doWork(int dataPipeId) {
  common::ErrorCode errorCode = common::ErrorCode::SUCCESS;

  common::ObjectMetadatas objectMetadatas;
  std::vector<int> inputPorts = getInputPorts();
  int inputPort = inputPorts[0];
  int outputPort = 0;
  if (!getSinkElementFlag()) {
    std::vector<int> outputPorts = getOutputPorts();
    int outputPort = outputPorts[0];
  }

  std::shared_ptr<void> data;
  while (getThreadStatus() == ThreadStatus::RUN) {
    data = popInputData(inputPort, dataPipeId);
    if (!data) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }
    break;
  }

  if (!data) return common::ErrorCode::SUCCESS;

  auto objectMetadata = std::static_pointer_cast<common::ObjectMetadata>(data);
  if (!(objectMetadata->mFrame->mEndOfStream) &&
      std::find(objectMetadata->mSkipElements.begin(),
                objectMetadata->mSkipElements.end(),
                getId()) == objectMetadata->mSkipElements.end()) {
    draw(objectMetadata);
    mFpsProfiler.add(1);
  }

  int channel_id_internal = objectMetadata->mFrame->mChannelIdInternal;
  int outDataPipeId =
      getSinkElementFlag()
          ? 0
          : (channel_id_internal % getOutputConnectorCapacity(outputPort));
  errorCode = pushOutputData(outputPort, outDataPipeId, objectMetadata);
  if (common::ErrorCode::SUCCESS != errorCode) {
    IVS_WARN(
        "Send data fail, element id: {0:d}, output port: {1:d}, data: "
        "{2:p}",
        getId(), outputPort, static_cast<void*>(objectMetadata.get()));
  }

  return common::ErrorCode::SUCCESS;
}

void Osd::draw(std::shared_ptr<common::ObjectMetadata> objectMetadata) {
  std::shared_ptr<bm_image> imageStorage;
  imageStorage.reset(new bm_image,
                     [&](bm_image* img) { bm_image_destroy(*img); });
  bm_image image = *(objectMetadata->mFrame->mSpData);

#if USE_OPENCV_DRAW_BOX
  cv::Mat frame_to_draw;
  cv::bmcv::toMAT(&image, frame_to_draw);
  switch (mOsdType) {
    case OsdType::DET:
      draw_opencv_det_result(objectMetadata, mClassNames, frame_to_draw, true);
      break;

    case OsdType::TRACK:
      draw_opencv_track_result(objectMetadata, mClassNames, frame_to_draw,
                               false);
      break;

    default:
      IVS_WARN("osd_type not support");
  }
  cv::bmcv::toBMI(frame_to_draw, &(*imageStorage));
  if ((*imageStorage).image_format != FORMAT_YUV420P) {
    bm_image frame;
    bm_image_create(objectMetadata->mFrame->mHandle, (*imageStorage).height,
                    (*imageStorage).width, FORMAT_YUV420P,
                    (*imageStorage).data_type, &frame);
    bmcv_image_storage_convert(objectMetadata->mFrame->mHandle, 1,
                               &(*imageStorage), &frame);
    bm_image_destroy(*imageStorage);
    *imageStorage = frame;
  }
#else
  bm_image_create(objectMetadata->mFrame->mHandle,
                  objectMetadata->mFrame->mHeight,
                  objectMetadata->mFrame->mWidth, FORMAT_YUV420P,
                  image.data_type, &(*imageStorage));
  bmcv_image_storage_convert(objectMetadata->mFrame->mHandle, 1, &image,
                             &(*imageStorage));
  switch (mOsdType) {
    case OsdType::DET:
      draw_bmcv_det_result(objectMetadata->mFrame->mHandle, objectMetadata,
                           mClassNames, *imageStorage, false);
      break;

    case OsdType::TRACK:
      draw_bmcv_track_result(objectMetadata->mFrame->mHandle, objectMetadata,
                             mClassNames, *imageStorage, false);
      break;

    default:
      IVS_WARN("osd_type not support");
  }
#endif
  objectMetadata->mFrame->mSpDataOsd = imageStorage;
}

REGISTER_WORKER("osd", Osd)

}  // namespace osd
}  // namespace element
}  // namespace sophon_stream
