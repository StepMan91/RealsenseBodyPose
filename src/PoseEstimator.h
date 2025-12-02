// GPU-Accelerated Pose Estimator using TensorRT

#pragma once

#include "Utils.h"
#include <opencv2/opencv.hpp>
#include <NvInfer.h>
#include <NvOnnxParser.h>
#include <cuda_runtime_api.h>
#include <memory>
#include <vector>
#include <string>

namespace RealsenseBodyPose {

/**
 * @brief GPU-accelerated pose estimation using TensorRT
 * 
 * Loads YOLOv8-Pose TensorRT engine and performs inference on RTX 4070
 */
class PoseEstimator {
public:
    /**
     * @brief Configuration for pose estimator
     */
    struct Config {
        std::string enginePath;         // Path to TensorRT engine file
        int inputWidth = 640;           // Model input width
        int inputHeight = 640;          // Model input height
        float confidenceThreshold = 0.5f;  // Minimum confidence for detection
        float nmsThreshold = 0.45f;     // Non-max suppression threshold
        int maxDetections = 10;         // Maximum number of people to detect
        
        Config() = default;
        explicit Config(const std::string& path) : enginePath(path) {}
    };
    
    /**
     * @brief Constructor
     * @param config Pose estimator configuration
     */
    explicit PoseEstimator(const Config& config);
    
    /**
     * @brief Destructor - cleanup GPU resources
     */
    ~PoseEstimator();
    
    /**
     * @brief Initialize TensorRT engine and allocate GPU memory
     * @throws std::runtime_error if initialization fails
     */
    void initialize();
    
    /**
     * @brief Run pose estimation on RGB image
     * @param image Input RGB image (CV_8UC3)
     * @return Vector of detected skeletons (2D keypoints only)
     */
    std::vector<Skeleton> estimate(const cv::Mat& image);
    
    /**
     * @brief Check if estimator is initialized
     * @return true if ready for inference
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief Get input dimensions
     */
    cv::Size getInputSize() const { return cv::Size(config_.inputWidth, config_.inputHeight); }

private:
    Config config_;
    bool initialized_;
    
    // TensorRT objects
    nvinfer1::IRuntime* runtime_;
    nvinfer1::ICudaEngine* engine_;
    nvinfer1::IExecutionContext* context_;
    
    // CUDA buffers
    void* buffers_[2];  // Input and output buffers on GPU
    cudaStream_t stream_;
    
    // Model metadata
    int inputIndex_;
    int outputIndex_;
    size_t inputSize_;
    size_t outputSize_;
    
    // Preprocessing parameters
    float scaleX_;
    float scaleY_;
    float padX_;
    float padY_;
    
    /**
     * @brief Load TensorRT engine from file
     */
    void loadEngine();
    
    /**
     * @brief Allocate GPU buffers for inference
     */
    void allocateBuffers();
    
    /**
     * @brief Preprocess image for model input
     * @param image Input image
     * @param blob Output preprocessed blob (GPU memory)
     */
    void preprocess(const cv::Mat& image, float* blob);
    
    /**
     * @brief Postprocess model output to extract skeletons
     * @param output Raw model output
     * @param imageWidth Original image width
     * @param imageHeight Original image height
     * @return Detected skeletons with 2D keypoints
     */
    std::vector<Skeleton> postprocess(const float* output, int imageWidth, int imageHeight);
    
    /**
     * @brief Apply Non-Maximum Suppression
     * @param skeletons Input skeletons
     * @return Filtered skeletons after NMS
     */
    std::vector<Skeleton> applyNMS(const std::vector<Skeleton>& skeletons);
    
    /**
     * @brief Calculate Intersection over Union for two bounding boxes
     */
    float calculateIoU(const float* box1, const float* box2);
};

} // namespace RealsenseBodyPose
