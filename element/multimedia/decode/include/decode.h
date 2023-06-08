#pragma once

#include "common/Graphics.hpp"
#include "decoder.h"
#include "element.h"

namespace sophon_stream {
namespace element {
namespace decode {

struct ChannelOperateRequest {
  enum class ChannelOperate {
    START,
    STOP,
    PAUSE,
    RESUME,
  };
  int channelId;
  ChannelOperate operation;
  std::string json;
};

struct ChannelOperateResponse {
  common::ErrorCode errorCode;
  std::string errorInfo;
};

struct ChannelTask {
  ChannelOperateRequest request;
  ChannelOperateResponse response;
};

class ThreadWrapper {
  friend class Decoder;

  using InitHandler = std::function<common::ErrorCode(void)>;
  using DataHandler = std::function<common::ErrorCode(void)>;
  using UninitHandler = std::function<common::ErrorCode(void)>;

  enum class ThreadStatus {
    RUN,
    STOP,
    PAUSE,
    RESUME,
  };

 public:
  ThreadWrapper() {}
  ~ThreadWrapper() {}
  void init(InitHandler initHandler, DataHandler dataHandler,
            UninitHandler uninitHandler) {
    mInitHandler = initHandler;
    mDataHandler = dataHandler;
    mUninitHandler = uninitHandler;
  }
  common::ErrorCode start() {
    IVS_INFO("Start element thread start, element id: {0:d}");

    if (ThreadStatus::STOP != mThreadStatus) {
      IVS_ERROR("Can not start, current thread status is not stop");
      return common::ErrorCode::THREAD_STATUS_ERROR;
    }

    mThreadStatus = ThreadStatus::RUN;

    mSpThread =
        std::make_shared<std::thread>(std::bind(&ThreadWrapper::run, this));

    IVS_INFO("Start element thread finish, element id: {0:d}");
    return common::ErrorCode::SUCCESS;
  }

  common::ErrorCode stop(bool sync = true) {
    IVS_INFO("Stop element thread start, element id: {0:d}");

    if (ThreadStatus::STOP == mThreadStatus) {
      IVS_ERROR("Can not stop, current thread status is stop");
      return common::ErrorCode::THREAD_STATUS_ERROR;
    }

    mThreadStatus = ThreadStatus::STOP;

    (sync) ? (mSpThread->join()) : (mSpThread->detach());

    mSpThread.reset();

    IVS_INFO("Stop element thread finish, element id: {0:d}");
    return common::ErrorCode::SUCCESS;
  }

  common::ErrorCode pause() {
    IVS_INFO("Pause element thread start, element id: {0:d}");

    if (ThreadStatus::RUN != mThreadStatus) {
      IVS_ERROR("Can not pause, current thread status is not run");
      return common::ErrorCode::THREAD_STATUS_ERROR;
    }

    mThreadStatus = ThreadStatus::PAUSE;

    IVS_INFO("Pause element thread finish, element id: {0:d}");
    return common::ErrorCode::SUCCESS;
  }

  common::ErrorCode resume() {
    IVS_INFO("Resume element thread start, element id: {0:d}");

    if (ThreadStatus::PAUSE != mThreadStatus) {
      IVS_ERROR("Can not resume, current thread status is not pause");
      return common::ErrorCode::THREAD_STATUS_ERROR;
    }

    mThreadStatus = ThreadStatus::RUN;

    IVS_INFO("Resume element thread finish, element id: {0:d}");
    return common::ErrorCode::SUCCESS;
  }

 private:
  ThreadWrapper(const ThreadWrapper&) = delete;
  ThreadWrapper& operator=(const ThreadWrapper&) = delete;
  void run() {
    common::ErrorCode ret = mInitHandler();
    while (ThreadStatus::STOP != mThreadStatus &&
           ret == common::ErrorCode::SUCCESS) {
      mDataHandler();
    }
    if (ret != common::ErrorCode::SUCCESS) {
      mUninitHandler();
    }
  }
  std::shared_ptr<std::thread> mSpThread = nullptr;
  ThreadStatus mThreadStatus = ThreadStatus::STOP;
  InitHandler mInitHandler{nullptr};
  DataHandler mDataHandler{nullptr};
  UninitHandler mUninitHandler{nullptr};
};

struct ChannelInfo {
  int mFrameCount = 0;
  std::shared_ptr<Decoder> mSpDecoder;
  std::shared_ptr<std::mutex> mMtx;
  std::shared_ptr<std::condition_variable> mCv;
  std::shared_ptr<ThreadWrapper> mThreadWrapper;
};

class Decode : public ::sophon_stream::framework::Element {
 public:
  Decode();
  ~Decode() override;

  Decode(const Decode&) = delete;
  Decode& operator=(const Decode&) = delete;
  Decode(Decode&&) = default;
  Decode& operator=(Decode&&) = default;

 private:
  common::ErrorCode initInternal(const std::string& json) override;
  void uninitInternal() override;
  void onStart() override;
  void onStop() override;
  common::ErrorCode doWork(int dataPipe) override;

  common::ErrorCode startTask(std::shared_ptr<ChannelTask>& channelTask);

  common::ErrorCode stopTask(std::shared_ptr<ChannelTask>& channelTask);

  common::ErrorCode pauseTask(std::shared_ptr<ChannelTask>& channelTask);

  common::ErrorCode resumeTask(std::shared_ptr<ChannelTask>& channelTask);

  common::ErrorCode process(const std::shared_ptr<ChannelTask>& channelTask,
                            const std::shared_ptr<ChannelInfo>& channelInfo);

 private:
  std::map<int, std::shared_ptr<ChannelInfo>> mThreadsPool;
  std::mutex mThreadsPoolMtx;
};

}  // namespace decode
}  // namespace element
}  // namespace sophon_stream