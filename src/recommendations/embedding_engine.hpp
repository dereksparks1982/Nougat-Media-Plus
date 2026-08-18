#pragma once

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
    std::string model_path_;
    void* model_ = nullptr;
    void* context_ = nullptr;
    bool initialized_ = false;
};

} // namespace reddmedia
