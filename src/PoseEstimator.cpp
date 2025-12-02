// Pose Estimator Implementation with TensorRT

#include "PoseEstimator.h"
#include <fstream>
#include <algorithm>
#include <numeric>

namespace RealsenseBodyPose {

// TensorRT logger
class Logger : public nvinfer1::ILogger {
    void log(Severity severity, const char* msg) noexcept override {
        if (severity <= Severity::kWARNING) {
            std::cout << "[TensorRT] " << msg << std::endl;
        }
    }
} gLogger;

PoseEstimator::PoseEstimator(const Config& config)
    : config_(config)
    , initialized_(false)
    , runtime_(nullptr)
    , engine_(nullptr)
    , context_(nullptr)
    , stream_(nullptr)
    , inputIndex_(-1)
    , outputIndex_(-1)
    , inputSize_(0)
    , outputSize_(0)
    , scaleX_(1.0f)
    , scaleY_(1.0f)
    , padX_(0.0f)
    , padY_(0.0f)
{
    buffers_[0] = nullptr;
    buffers_[1] = nullptr;
}

PoseEstimator::~PoseEstimator() {
    // Free CUDA buffers
    if (buffers_[0]) cudaFree(buffers_[0]);
    if (buffers_[1]) cudaFree(buffers_[1]);
    if (stream_) cudaStreamDestroy(stream_);
    
    // Destroy TensorRT objects
    if (context_) context_->destroy();
    if (engine_) engine_->destroy();
    if (runtime_) runtime_->destroy();
}

void PoseEstimator::loadEngine() {
    log(LogLevel::INFO, "Loading TensorRT engine: " + config_.enginePath);
    
    // Read engine file
    std::ifstream file(config_.enginePath, std::ios::binary);
    if (!file.good()) {
        throw std::runtime_error("Cannot open engine file: " + config_.enginePath);
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> engineData(size);
    file.read(engineData.data(), size);
    file.close();
    
    // Create runtime and deserialize engine
    runtime_ = nvinfer1::createInferRuntime(gLogger);
    if (!runtime_) {
        throw std::runtime_error("Failed to create TensorRT runtime");
    }
    
    engine_ = runtime_->deserializeCudaEngine(engineData.data(), size);
    if (!engine_) {
        throw std::runtime_error("Failed to deserialize CUDA engine");
    }
    
    context_ = engine_->createExecutionContext();
    if (!context_) {
        throw std::runtime_error("Failed to create execution context");
    }
    
    log(LogLevel::INFO, "✅ TensorRT engine loaded successfully");
    log(LogLevel::INFO, "  Engine size: " + std::to_string(size / (1024.0 * 1024.0)) + " MB");
    log(LogLevel::INFO, "  Max batch size: " + std::to_string(engine_->getMaxBatchSize()));
}

void PoseEstimator::allocateBuffers() {
    // Get input/output binding indices
    inputIndex_ = engine_->getBindingIndex("images");  // YOLOv8 input name
    outputIndex_ = engine_->getBindingIndex("output0"); // YOLOv8 output name
    
    if (inputIndex_ < 0 || outputIndex_ < 0) {
        throw std::runtime_error("Invalid binding indices");
    }
    
    // Calculate buffer sizes
    auto inputDims = engine_->getBindingDimensions(inputIndex_);
    auto outputDims = engine_->getBindingDimensions(outputIndex_);
    
    // Input: [1, 3, 640, 640] - batch, channels, height, width
    inputSize_ = 1 * 3 * config_.inputHeight * config_.inputWidth * sizeof(float);
    
    // Output: [1, 56, 8400] - batch, (4 bbox + 1 conf + 51 keypoints), anchors
    // For YOLOv8-Pose: 56 = 4 (bbox) + 1 (conf) + 51 (17 keypoints * 3)
    outputSize_ = outputDims.d[1] * outputDims.d[2] * sizeof(float);
    
    // Allocate GPU memory
    cudaMalloc(&buffers_[inputIndex_], inputSize_);
    cudaMalloc(&buffers_[outputIndex_], outputSize_);
    cudaStreamCreate(&stream_);
    
    log(LogLevel::INFO, "✅ GPU buffers allocated:");
    log(LogLevel::INFO, "  Input:  " + std::to_string(inputSize_ / (1024.0 * 1024.0)) + " MB");
    log(LogLevel::INFO, "  Output: " + std::to_string(outputSize_ / (1024.0 * 1024.0)) + " MB");
}

void PoseEstimator::initialize() {
    try {
        loadEngine();
        allocateBuffers();
        
        // Test CUDA availability
        int deviceCount;
        cudaGetDeviceCount(&deviceCount);
        if (deviceCount == 0) {
            throw std::runtime_error("No CUDA-capable GPU found");
        }
        
        cudaDeviceProp deviceProp;
        cudaGetDeviceProperties(&deviceProp, 0);
        
        log(LogLevel::INFO, "✅ GPU Acceleration Enabled:");
        log(LogLevel::INFO, "  GPU: " + std::string(deviceProp.name));
        log(LogLevel::INFO, "  Compute Capability: " + std::to_string(deviceProp.major) + "." + 
                           std::to_string(deviceProp.minor));
        log(LogLevel::INFO, "  Memory: " + std::to_string(deviceProp.totalGlobalMem / (1024 * 1024)) + " MB");
        
        initialized_ = true;
        log(LogLevel::INFO, "✅ Pose estimator initialized successfully");
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Pose estimator initialization failed: " + std::string(e.what()));
    }
}

void PoseEstimator::preprocess(const cv::Mat& image, float* blob) {
    // Letterbox resize to maintain aspect ratio
    int imgWidth = image.cols;
    int imgHeight = image.rows;
    
    float scale = std::min(config_.inputWidth / (float)imgWidth, 
                          config_.inputHeight / (float)imgHeight);
    
    int newWidth = static_cast<int>(imgWidth * scale);
    int newHeight = static_cast<int>(imgHeight * scale);
    
    scaleX_ = (float)imgWidth / newWidth;
    scaleY_ = (float)imgHeight / newHeight;
    padX_ = (config_.inputWidth - newWidth) / 2.0f;
    padY_ = (config_.inputHeight - newHeight) / 2.0f;
    
    // Resize image
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(newWidth, newHeight));
    
    // Create padded image (letterbox)
    cv::Mat padded = cv::Mat::zeros(config_.inputHeight, config_.inputWidth, CV_8UC3);
    resized.copyTo(padded(cv::Rect(padX_, padY_, newWidth, newHeight)));
    
    // Convert BGR to RGB and normalize to [0, 1]
    cv::Mat rgb;
    cv::cvtColor(padded, rgb, cv::COLOR_BGR2RGB);
    rgb.convertTo(rgb, CV_32FC3, 1.0 / 255.0);
    
    // Convert HWC to CHW format (channels first) and copy to blob
    std::vector<cv::Mat> channels(3);
    cv::split(rgb, channels);
    
    int channelSize = config_.inputWidth * config_.inputHeight;
    for (int c = 0; c < 3; ++c) {
        memcpy(blob + c * channelSize, channels[c].data, channelSize * sizeof(float));
    }
}

std::vector<Skeleton> PoseEstimator::postprocess(const float* output, int imageWidth, int imageHeight) {
    std::vector<Skeleton> skeletons;
    
    // YOLOv8-Pose output format: [56, 8400]
    // 56 = 4 (bbox) + 1 (conf) + 51 (17 keypoints * 3: x, y, visibility)
    const int numAnchors = 8400;
    const int numElements = 56;
    
    for (int i = 0; i < numAnchors; i++) {
        float confidence = output[4 * numAnchors + i];
        
        if (confidence < config_.confidenceThreshold) {
            continue;
        }
        
        // Parse bounding box (cx, cy, w, h)
        float cx = output[0 * numAnchors + i];
        float cy = output[1 * numAnchors + i];
        float w = output[2 * numAnchors + i];
        float h = output[3 * numAnchors + i];
        
        // Convert to (x1, y1, x2, y2) and scale back to original image
        float x1 = (cx - w / 2 - padX_) * scaleX_;
        float y1 = (cy - h / 2 - padY_) * scaleY_;
        float x2 = (cx + w / 2 - padX_) * scaleX_;
        float y2 = (cy + h / 2 - padY_) * scaleY_;
        
        // Clamp to image bounds
        x1 = std::max(0.0f, std::min(x1, (float)imageWidth));
        y1 = std::max(0.0f, std::min(y1, (float)imageHeight));
        x2 = std::max(0.0f, std::min(x2, (float)imageWidth));
        y2 = std::max(0.0f, std::min(y2, (float)imageHeight));
        
        // Create skeleton
        Skeleton skeleton;
        skeleton.bbox[0] = x1;
        skeleton.bbox[1] = y1;
        skeleton.bbox[2] = x2 - x1;
        skeleton.bbox[3] = y2 - y1;
        skeleton.overallConfidence = confidence;
        
        // Parse 17 keypoints
        for (int j = 0; j < 17; j++) {
            float kptX = output[(5 + j * 3) * numAnchors + i];
            float kptY = output[(6 + j * 3) * numAnchors + i];
            float kptConf = output[(7 + j * 3) * numAnchors + i];
            
            // Scale back to original image coordinates
            kptX = (kptX - padX_) * scaleX_;
            kptY = (kptY - padY_) * scaleY_;
            
            skeleton.keypoints2D[j] = Keypoint2D(kptX, kptY, kptConf);
        }
        
        skeletons.push_back(skeleton);
    }
    
    // Apply Non-Maximum Suppression
    return applyNMS(skeletons);
}

float PoseEstimator::calculateIoU(const float* box1, const float* box2) {
    float x1 = std::max(box1[0], box2[0]);
    float y1 = std::max(box1[1], box2[1]);
    float x2 = std::min(box1[0] + box1[2], box2[0] + box2[2]);
    float y2 = std::min(box1[1] + box1[3], box2[1] + box2[3]);
    
    float intersection = std::max(0.0f, x2 - x1) * std::max(0.0f, y2 - y1);
    float area1 = box1[2] * box1[3];
    float area2 = box2[2] * box2[3];
    float unionArea = area1 + area2 - intersection;
    
    return intersection / unionArea;
}

std::vector<Skeleton> PoseEstimator::applyNMS(const std::vector<Skeleton>& skeletons) {
    if (skeletons.empty()) return {};
    
    // Sort by confidence (descending)
    std::vector<size_t> indices(skeletons.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        return skeletons[a].overallConfidence > skeletons[b].overallConfidence;
    });
    
    std::vector<bool> suppressed(skeletons.size(), false);
    std::vector<Skeleton> result;
    
    for (size_t i = 0; i < indices.size(); i++) {
        size_t idx = indices[i];
        if (suppressed[idx]) continue;
        
        result.push_back(skeletons[idx]);
        
        if (result.size() >= config_.maxDetections) break;
        
        for (size_t j = i + 1; j < indices.size(); j++) {
            size_t idx2 = indices[j];
            if (suppressed[idx2]) continue;
            
            float iou = calculateIoU(skeletons[idx].bbox, skeletons[idx2].bbox);
            if (iou > config_.nmsThreshold) {
                suppressed[idx2] = true;
            }
        }
    }
    
    return result;
}

std::vector<Skeleton> PoseEstimator::estimate(const cv::Mat& image) {
    if (!initialized_) {
        throw std::runtime_error("Pose estimator not initialized");
    }
    
    // Allocate host memory for input
    std::vector<float> inputBlob(config_.inputWidth * config_.inputHeight * 3);
    
    // Preprocess image
    preprocess(image, inputBlob.data());
    
    // Copy input to GPU
    cudaMemcpyAsync(buffers_[inputIndex_], inputBlob.data(), inputSize_, 
                    cudaMemcpyHostToDevice, stream_);
    
    // Run inference
    context_->enqueueV2(buffers_, stream_, nullptr);
    
    // Allocate host memory for output
    std::vector<float> outputBlob(outputSize_ / sizeof(float));
    
    // Copy output from GPU
    cudaMemcpyAsync(outputBlob.data(), buffers_[outputIndex_], outputSize_,
                    cudaMemcpyDeviceToHost, stream_);
    
    // Synchronize stream
    cudaStreamSynchronize(stream_);
    
    // Postprocess to get skeletons
    return postprocess(outputBlob.data(), image.cols, image.rows);
}

} // namespace RealsenseBodyPose
