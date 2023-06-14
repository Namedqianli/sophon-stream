//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-STREAM is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#ifndef SOPHON_STREAM_ELEMENT_BYTETRACK_H_
#define SOPHON_STREAM_ELEMENT_BYTETRACK_H_

#include "bytetrack_bytetracker.h"

namespace sophon_stream {
namespace element {
namespace bytetrack {

struct BytetrackContext {
  float trackThresh;
  float highThresh;
  float matchThresh;
  int frameRate;
  int trackBuffer;
};

class Bytetrack : public ::sophon_stream::framework::Element {
 public:
  Bytetrack();
  ~Bytetrack() override;

  common::ErrorCode initInternal(const std::string& json) override;
  void uninitInternal() override;

  common::ErrorCode doWork(int dataPipeId) override;

  static constexpr const char* CONFIG_INTERNAL_FRAME_RATE_FIELD = "frame_rate";
  static constexpr const char* CONFIG_INTERNAL_TRACK_BUFFER_FIELD =
      "track_buffer";
  static constexpr const char* CONFIG_INTERNAL_TRACK_THRESH_FIELD =
      "track_thresh";
  static constexpr const char* CONFIG_INTERNAL_HIGH_THRESH_FIELD =
      "high_thresh";
  static constexpr const char* CONFIG_INTERNAL_MATCH_THRESH_FIELD =
      "match_thresh";

 private:
  std::shared_ptr<BytetrackContext> mContext;  // context对象

  std::map<int, std::shared_ptr<BYTETracker>> mByteTrackerMap;

  common::ErrorCode initContext(const std::string& json);
  void process(int dataPipeId,
               std::shared_ptr<common::ObjectMetadata>& objectMetadata);
};

}  // namespace bytetrack
}  // namespace element
}  // namespace sophon_stream

#endif  // SOPHON_STREAM_ELEMENT_BYTETRACK_H_