/**
 * @file data_utils.h
 * @brief Common utilities for file I/O and timing in simple AscendC operators.
 */

#ifndef DATA_UTILS_H
#define DATA_UTILS_H

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <cmath>

#define ERROR_LOG(fmt, args...) fprintf(stdout, "[ERROR]  " fmt "\n", ##args)
#define INFO_LOG(fmt, args...)  fprintf(stdout, "[INFO]   " fmt "\n", ##args)

/**
 * @brief Read binary data from file
 */
inline bool ReadFile(const std::string &filePath, size_t &fileSize, void *buffer, size_t bufferSize)
{
    struct stat sBuf;
    if (stat(filePath.data(), &sBuf) == -1) {
        ERROR_LOG("failed to stat file %s", filePath.c_str());
        return false;
    }
    if (!S_ISREG(sBuf.st_mode)) {
        ERROR_LOG("%s is not a regular file", filePath.c_str());
        return false;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        ERROR_LOG("failed to open file %s", filePath.c_str());
        return false;
    }

    std::filebuf *buf = file.rdbuf();
    size_t size = buf->pubseekoff(0, std::ios::end, std::ios::in);
    if (size == 0) {
        ERROR_LOG("file %s is empty", filePath.c_str());
        file.close();
        return false;
    }
    if (size > bufferSize) {
        ERROR_LOG("file size %zu > buffer size %zu", size, bufferSize);
        file.close();
        return false;
    }
    buf->pubseekpos(0, std::ios::in);
    buf->sgetn(static_cast<char *>(buffer), size);
    fileSize = size;
    file.close();
    return true;
}

/**
 * @brief Write binary data to file
 */
inline bool WriteFile(const std::string &filePath, const void *buffer, size_t size)
{
    if (buffer == nullptr) {
        ERROR_LOG("Write file failed: null buffer");
        return false;
    }

    int fd = open(filePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWRITE);
    if (fd < 0) {
        ERROR_LOG("failed to open file %s for writing", filePath.c_str());
        return false;
    }

    size_t writeSize = write(fd, buffer, size);
    close(fd);
    if (writeSize != size) {
        ERROR_LOG("Write file failed: wrote %zu / %zu bytes", writeSize, size);
        return false;
    }
    return true;
}

/**
 * @brief Get current time in microseconds
 */
inline double GetTimeUs()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1e6 + tv.tv_usec;
}

/**
 * @brief Verify float results against golden data (CPU reference)
 */
inline bool VerifyResult(const float *result, const float *golden, size_t count, float rtol = 1e-3f, float atol = 1e-5f)
{
    size_t errors = 0;
    float maxError = 0.0f;
    for (size_t i = 0; i < count; i++) {
        float diff = std::fabs(result[i] - golden[i]);
        float threshold = atol + rtol * std::fabs(golden[i]);
        if (diff > threshold) {
            if (errors < 10) {
                ERROR_LOG("Mismatch at [%zu]: result=%.6f, golden=%.6f, diff=%.6e", i, result[i], golden[i], diff);
            }
            errors++;
            if (diff > maxError) maxError = diff;
        }
    }
    if (errors > 0) {
        ERROR_LOG("Total %zu / %zu mismatches, max error = %.6e", errors, count, maxError);
        return false;
    }
    INFO_LOG("Verification PASSED: all %zu elements match (rtol=%.e, atol=%.e)", count, rtol, atol);
    return true;
}

#endif // DATA_UTILS_H
