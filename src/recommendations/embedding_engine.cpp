#include "embedding_engine.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <thread>

#if !defined(REDDMEDIA_AI_STUB)
#include <llama.h>
#endif

namespace reddmedia {
namespace {

void normalize(std::vector<float>& values) {
    double length_squared = 0.0;
    for (const float value : values) length_squared += static_cast<double>(value) * value;
    if (length_squared <= 0.0) return;
    const float inverse = static_cast<float>(1.0 / std::sqrt(length_squared));
    for (float& value : values) value *= inverse;
}

#if defined(REDDMEDIA_AI_STUB)
std::vector<float> deterministic_embedding(const std::string& text) {
    std::vector<float> result(64U, 0.0F);
    std::uint64_t state = 1469598103934665603ULL;
    for (const unsigned char character : text) {
        state ^= character;
        state *= 1099511628211ULL;
        const std::size_t index = static_cast<std::size_t>(state % result.size());
        result[index] += (state & 1ULL) != 0ULL ? 1.0F : -1.0F;
    }
    normalize(result);
    return result;
}
#endif

} // namespace

EmbeddingEngine::EmbeddingEngine(std::string model_path)
    : model_path_(std::move(model_path)) {}

EmbeddingEngine::~EmbeddingEngine() {
#if !defined(REDDMEDIA_AI_STUB)
    if (context_) llama_free(static_cast<llama_context*>(context_));
    if (model_) llama_model_free(static_cast<llama_model*>(model_));
    if (initialized_) llama_backend_free();
#endif
}

bool EmbeddingEngine::initialize(std::string& error) {
    if (initialized_) return true;
#if defined(REDDMEDIA_AI_STUB)
    (void)error;
    initialized_ = true;
    return true;
#else
    std::ifstream input(model_path_, std::ios::binary);
    if (!input) {
        error = "ReddMedia's pinned offline embedding model is missing.";
        return false;
    }
    llama_backend_init();
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 0;
    llama_model* model = llama_model_load_from_file(model_path_.c_str(), model_params);
    if (!model) {
        error = "ReddMedia could not load its pinned offline embedding model.";
        llama_backend_free();
        return false;
    }
    llama_context_params context_params = llama_context_default_params();
    context_params.n_ctx = 512;
    context_params.n_batch = 512;
    context_params.n_ubatch = 512;
    context_params.n_seq_max = 1;
    const unsigned hardware_threads = std::max(1U, std::thread::hardware_concurrency());
    context_params.n_threads = static_cast<int>(std::min(8U, hardware_threads));
    context_params.n_threads_batch = context_params.n_threads;
    context_params.embeddings = true;
    context_params.pooling_type = LLAMA_POOLING_TYPE_MEAN;
    context_params.attention_type = LLAMA_ATTENTION_TYPE_NON_CAUSAL;
    llama_context* context = llama_init_from_model(model, context_params);
    if (!context) {
        llama_model_free(model);
        llama_backend_free();
        error = "ReddMedia could not initialize offline embedding inference.";
        return false;
    }
    model_ = model;
    context_ = context;
    initialized_ = true;
    return true;
#endif
}

bool EmbeddingEngine::embed_document(const std::string& text,
                                     std::vector<float>& embedding,
                                     std::string& error) {
    if (!initialize(error)) return false;
    const std::string prompt = "search_document: " + text;
#if defined(REDDMEDIA_AI_STUB)
    embedding = deterministic_embedding(prompt);
    return true;
#else
    llama_model* model = static_cast<llama_model*>(model_);
    llama_context* context = static_cast<llama_context*>(context_);
    const llama_vocab* vocabulary = llama_model_get_vocab(model);
    int count = llama_tokenize(vocabulary, prompt.c_str(),
                               static_cast<int>(prompt.size()), nullptr, 0, true, true);
    if (count == 0) {
        error = "The offline embedding model received empty metadata.";
        return false;
    }
    if (count < 0) count = -count;
    if (count > 512) count = 512;
    std::vector<llama_token> tokens(static_cast<std::size_t>(count));
    int written = llama_tokenize(vocabulary, prompt.c_str(),
                                 static_cast<int>(prompt.size()), tokens.data(), count,
                                 true, true);
    if (written <= 0) {
        error = "The offline embedding model could not tokenize metadata.";
        return false;
    }
    tokens.resize(static_cast<std::size_t>(written));
    llama_batch batch = llama_batch_init(written, 0, 1);
    batch.n_tokens = written;
    for (int index = 0; index < written; ++index) {
        batch.token[index] = tokens[static_cast<std::size_t>(index)];
        batch.pos[index] = index;
        batch.n_seq_id[index] = 1;
        batch.seq_id[index][0] = 0;
        batch.logits[index] = 1;
    }
    llama_memory_t memory = llama_get_memory(context);
    if (memory) llama_memory_clear(memory, true);
    const int result = llama_model_has_encoder(model)
        ? llama_encode(context, batch)
        : llama_decode(context, batch);
    if (result != 0) {
        llama_batch_free(batch);
        error = "The offline embedding model could not process metadata.";
        return false;
    }
    const float* values = llama_get_embeddings_seq(context, 0);
    const int dimensions = llama_model_n_embd_out(model);
    if (!values || dimensions <= 0) {
        llama_batch_free(batch);
        error = "The offline embedding model returned no embedding.";
        return false;
    }
    embedding.assign(values, values + dimensions);
    llama_batch_free(batch);
    normalize(embedding);
    return true;
#endif
}

bool EmbeddingEngine::using_real_model() const {
#if defined(REDDMEDIA_AI_STUB)
    return false;
#else
    return initialized_;
#endif
}

float EmbeddingEngine::cosine_similarity(const std::vector<float>& left,
                                         const std::vector<float>& right) {
    if (left.empty() || left.size() != right.size()) return -1.0F;
    double dot = 0.0;
    double left_length = 0.0;
    double right_length = 0.0;
    for (std::size_t index = 0; index < left.size(); ++index) {
        dot += static_cast<double>(left[index]) * right[index];
        left_length += static_cast<double>(left[index]) * left[index];
        right_length += static_cast<double>(right[index]) * right[index];
    }
    if (left_length <= 0.0 || right_length <= 0.0) return -1.0F;
    return static_cast<float>(dot / std::sqrt(left_length * right_length));
}

} // namespace reddmedia
