#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/core/cuda.hpp>
#include <iostream>

int main() {
    std::cout << "=== OpenCV GPU Support Diagnostic ===" << std::endl;
    std::cout << "OpenCV Version: " << CV_VERSION << std::endl;
    
    // Check CUDA support
    int cudaDevices = cv::cuda::getCudaEnabledDeviceCount();
    std::cout << "\nCUDA Devices: " << cudaDevices << std::endl;
    
    if (cudaDevices > 0) {
        cv::cuda::DeviceInfo deviceInfo;
        std::cout << "GPU Name: " << deviceInfo.name() << std::endl;
        std::cout << "Compute Capability: " << deviceInfo.majorVersion() << "." << deviceInfo.minorVersion() << std::endl;
    }
    
    // Check DNN backends
    std::cout << "\n=== Available DNN Backends ===" << std::endl;
    auto backends = cv::dnn::getAvailableBackends();
    for (const auto& backend : backends) {
        std::cout << "Backend: " << backend.first << ", Target: " << backend.second << std::endl;
    }
    
    // Check build info
    std::cout << "\n=== OpenCV Build Info ===" << std::endl;
    std::cout << cv::getBuildInformation() << std::endl;
    
    return 0;
}
