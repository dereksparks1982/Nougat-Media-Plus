#pragma once

#include <mutex>
#include <string>
#include <vector>

namespace reddmedia {

class EmbeddingEngine {
public:
    explicit EmbeddingEngine(std::string model_path);
    ~EmbeddingEngine();

    EmbeddingEngine(const EmbeddingEngine&) = delete;
    EmbeddingEngine& operator=(const EmbeddingEngine&) = delete;

    bool initialize(std::string& error);
    bool embed_document(const std::string& text,
                        std::vector<float>& embedding,
                        std::string& error);
    bool using_real_model() const;

    static float cosine_similarity(const std::vector<float>& left,
                                   const std::vector<float>& right);

private:
    bool load_cached_embedding(const std::string& prompt, std::vector<float>& embedding) const;
    void save_cached_embedding(const std::string& prompt, const std::vector<float>& embedding) const;
    std::string cache_path_for_prompt(const std::string& prompt) const;

    std::string model_path_;
    std::string cache_dir_;
    mutable std::mutex inference_mutex_;
    void* model_ = nullptr;
    void* context_ = nullptr;
    bool initialized_ = false;
};

} // namespace reddmedia
