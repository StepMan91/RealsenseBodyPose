#include "DataRecorder.h"
#include <ctime>
#include <direct.h> // For _mkdir on Windows
#include <iomanip>
#include <iostream>
#include <sstream>


namespace RealsenseBodyPose {

DataRecorder::DataRecorder() : isRecording_(false), frameCount_(0) {
  // Ensure recordings directory exists
  _mkdir("recordings");
}

DataRecorder::~DataRecorder() { stop(); }

bool DataRecorder::start() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (isRecording_) {
    return true;
  }

  std::string timestamp = getTimestampString();
  currentFilePath_ = "recordings/recording_" + timestamp + ".csv";

  file_.open(currentFilePath_);
  if (!file_.is_open()) {
    std::cerr << "Failed to create recording file: " << currentFilePath_
              << std::endl;
    return false;
  }

  // Write CSV Header
  file_ << "Timestamp,FrameIndex,PersonID,Confidence,";
  // 17 Keypoints * (X, Y, Z, Conf)
  for (int i = 0; i < 17; i++) {
    file_ << "J" << i << "_X,"
          << "J" << i << "_Y,"
          << "J" << i << "_Z,"
          << "J" << i << "_Conf";
    if (i < 16)
      file_ << ",";
  }
  file_ << "\n";

  isRecording_ = true;
  frameCount_ = 0;
  std::cout << "[REC] Started recording to " << currentFilePath_ << std::endl;

  return true;
}

void DataRecorder::stop() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (isRecording_) {
    file_.close();
    isRecording_ = false;
    std::cout << "[REC] Stopped recording. Saved " << frameCount_ << " frames."
              << std::endl;
  }
}

bool DataRecorder::isRecording() const { return isRecording_; }

void DataRecorder::record(const std::vector<Skeleton> &skeletons) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!isRecording_ || !file_.is_open()) {
    return;
  }

  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now.time_since_epoch())
                       .count();

  for (size_t i = 0; i < skeletons.size(); i++) {
    const auto &skel = skeletons[i];

    file_ << timestamp << "," << frameCount_ << "," << i
          << "," // Person ID (just index for now)
          << std::fixed << std::setprecision(4) << skel.overallConfidence
          << ",";

    // Write 3D keypoints
    for (size_t k = 0; k < skel.keypoints3D.size(); k++) {
      const auto &kp = skel.keypoints3D[k];
      file_ << kp.x << "," << kp.y << "," << kp.z << "," << kp.confidence;
      if (k < skel.keypoints3D.size() - 1)
        file_ << ",";
    }
    file_ << "\n";
  }

  frameCount_++;
}

std::string DataRecorder::getTimestampString() const {
  auto now = std::chrono::system_clock::now();
  std::time_t now_c = std::chrono::system_clock::to_time_t(now);
  std::tm now_tm;
  localtime_s(&now_tm, &now_c);

  std::stringstream ss;
  ss << std::put_time(&now_tm, "%Y%m%d_%H%M%S");
  return ss.str();
}

} // namespace RealsenseBodyPose
