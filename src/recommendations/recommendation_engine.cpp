#include "recommendation_engine.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <random>
#include <set>
#include <sstream>

namespace reddmedia {
namespace {

std::string normalized_title(const MediaDescriptor& item) {
    std::string result;
    for (const unsigned char character : item.title) {
        if (std::isalnum(character) != 0) {
            result.push_back(static_cast<char>(std::tolower(character)));
        }
    }
    if (item.year > 0) result += ":" + std::to_string(item.year);
    return result;
}

std::string metadata_text(const MediaDescriptor& item) {
    std::ostringstream text;
    text << media_type_name(item.media_type) << ". Title: " << item.title;
    if (item.year > 0) text << ". Year: " << item.year;
    if (!item.genres.empty()) {
        text << ". Genres: ";
        for (std::size_t index = 0; index < item.genres.size(); ++index) {
            if (index > 0U) text << ", ";
            text << item.genres[index];
        }
    }
    if (!item.overview.empty()) text << ". Overview: " << item.overview;
    return text.str();
}

std::vector<MediaDescriptor> matching_local_items(
    RecommendationMediaType type,
    const std::vector<MediaDescriptor>& local_items) {
    std::vector<MediaDescriptor> result;
    for (const MediaDescriptor& item : local_items) {
        if (item.media_type == type && !item.id.empty()) result.push_back(item);
    }
    return result;
}

void filter_owned(std::vector<MediaDescriptor>& candidates,
                  const std::vector<MediaDescriptor>& local_items) {
    std::set<std::string> tmdb_ids;
    std::set<std::string> titles;
    for (const MediaDescriptor& item : local_items) {
        const std::string prefix = item.media_type == RecommendationMediaType::Movie
            ? "movie:" : "tv:";
        if (!item.tmdb_id.empty()) tmdb_ids.insert(prefix + item.tmdb_id);
        titles.insert(prefix + normalized_title(item));
    }
    candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
        [&tmdb_ids, &titles](const MediaDescriptor& item) {
            const std::string prefix = item.media_type == RecommendationMediaType::Movie
                ? "movie:" : "tv:";
            return (!item.tmdb_id.empty() && tmdb_ids.count(prefix + item.tmdb_id) > 0U) ||
                   titles.count(prefix + normalized_title(item)) > 0U;
        }), candidates.end());
}

} // namespace

RecommendationEngine::RecommendationEngine(std::string model_path,
                                           std::string history_path,
                                           std::string tmdb_token_path)
    : history_(std::move(history_path)),
      embeddings_(std::move(model_path)),
      tmdb_(std::move(tmdb_token_path)) {}

bool RecommendationEngine::record_started(const MediaDescriptor& item, std::string& error) {
    return history_.record_started(item, error);
}

bool RecommendationEngine::record_completed(const MediaDescriptor& item, std::string& error) {
    return history_.record_completed(item, error);
}

bool RecommendationEngine::recent_history(RecommendationMediaType type,
                                          std::vector<ViewingRecord>& records,
                                          std::string& error,
                                          int limit) {
    return history_.recent(type, records, error, limit);
}

bool RecommendationEngine::external_credential_available() const {
    return tmdb_.has_credential();
}

std::string RecommendationEngine::external_credential_label() const {
    return tmdb_.credential_label();
}

bool RecommendationEngine::test_external_credential(std::string& error) const {
    return tmdb_.test_saved_credential(error);
}

bool RecommendationEngine::save_external_credential(const std::string& credential,
                                                     std::string& error) {
    return tmdb_.save_credential(credential, error);
}

bool RecommendationEngine::clear_external_credential(std::string& error) {
    return tmdb_.clear_credential(error);
}

bool RecommendationEngine::load_external_poster_bmp(const std::string& poster_path,
                                                     int width,
                                                     int height,
                                                     std::string& bytes,
                                                     std::string& error) const {
    return tmdb_.load_poster_bmp(poster_path, width, height, bytes, error);
}

bool RecommendationEngine::load_watch_availability(RecommendationMediaType type,
                                                    const std::string& tmdb_id,
                                                    const std::string& region,
                                                    WatchAvailability& availability,
                                                    std::string& error) const {
    return tmdb_.watch_availability(type, tmdb_id, region, availability, error);
}

bool RecommendationEngine::load_watch_provider_catalog(
    const std::string& region,
    std::vector<WatchProvider>& providers,
    std::string& error) const {
    return tmdb_.watch_provider_catalog(region, providers, error);
}

bool RecommendationEngine::load_tv_episode_details(const std::string& series_tmdb_id,
                                                    int season_number,
                                                    int episode_number,
                                                    std::string& title,
                                                    std::string& overview,
                                                    std::string& error) const {
    return tmdb_.tv_episode_details(series_tmdb_id, season_number, episode_number,
                                    title, overview, error);
}

bool RecommendationEngine::load_tv_poster_path(const std::string& series_tmdb_id,
                                                int season_number,
                                                std::string& poster_path,
                                                std::string& error) const {
    return tmdb_.tv_poster_path(series_tmdb_id, season_number, poster_path, error);
}

bool RecommendationEngine::load_movie_poster_path(const std::string& tmdb_id,
                                                   std::string& poster_path,
                                                   std::string& error) const {
    return tmdb_.movie_poster_path(tmdb_id, poster_path, error);
}

bool RecommendationEngine::external_candidates(
    const RecommendationRequest& request,
    const std::vector<MediaDescriptor>& local_items,
    std::vector<MediaDescriptor>& candidates,
    std::string& error) {
    std::random_device device;
    std::mt19937 generator(device());
    int total_pages = 1;
    std::vector<MediaDescriptor> loaded;
    if (!tmdb_.discover(request.media_type, 1, loaded, total_pages, error)) return false;
    candidates.insert(candidates.end(), loaded.begin(), loaded.end());
    if (request.mode == RecommendationMode::Random) {
        std::uniform_int_distribution<int> page_distribution(1, std::max(1, total_pages));
        const int page = page_distribution(generator);
        if (page != 1) {
            loaded.clear();
            if (!tmdb_.discover(request.media_type, page, loaded, total_pages, error)) return false;
            candidates = std::move(loaded);
        }
    } else {
        for (int page = 2; page <= std::min(3, total_pages); ++page) {
            loaded.clear();
            if (!tmdb_.discover(request.media_type, page, loaded, total_pages, error)) return false;
            candidates.insert(candidates.end(), loaded.begin(), loaded.end());
        }
    }
    filter_owned(candidates, local_items);
    candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
        [&request](const MediaDescriptor& item) {
            return item.media_type != request.media_type;
        }), candidates.end());
    if (candidates.empty()) {
        error = "No unowned External title matched this request.";
        return false;
    }
    return true;
}

bool RecommendationEngine::usual_recommendation(
    const RecommendationRequest& request,
    const std::vector<MediaDescriptor>& candidates,
    RecommendationResult& result,
    std::string& error) {
    std::vector<ViewingRecord> history;
    if (!history_.recent(request.media_type, history, error, 25)) return false;
    if (history.empty()) {
        error = std::string("Watch a ") + media_type_name(request.media_type) +
            " in ReddMedia before asking for a Usual recommendation.";
        return false;
    }
    std::vector<float> profile;
    double total_weight = 0.0;
    for (std::size_t index = 0; index < history.size(); ++index) {
        std::vector<float> embedding;
        if (!embeddings_.embed_document(metadata_text(history[index].item), embedding, error)) {
            return false;
        }
        if (profile.empty()) profile.assign(embedding.size(), 0.0F);
        if (profile.size() != embedding.size()) continue;
        const double recency = 1.0 / (1.0 + static_cast<double>(index) * 0.12);
        const double repeats = 1.0 + std::log1p(std::max(0, history[index].play_count - 1)) * 0.2;
        const double weight = recency * repeats;
        for (std::size_t dimension = 0; dimension < profile.size(); ++dimension) {
            profile[dimension] += embedding[dimension] * static_cast<float>(weight);
        }
        total_weight += weight;
    }
    if (profile.empty() || total_weight <= 0.0) {
        error = "ReddMedia could not build a viewing profile from history.";
        return false;
    }
    for (float& value : profile) value /= static_cast<float>(total_weight);

    float best_score = -std::numeric_limits<float>::infinity();
    const MediaDescriptor* best = nullptr;
    for (const MediaDescriptor& candidate : candidates) {
        std::vector<float> embedding;
        if (!embeddings_.embed_document(metadata_text(candidate), embedding, error)) return false;
        const float score = EmbeddingEngine::cosine_similarity(profile, embedding);
        if (!best || score > best_score) {
            best = &candidate;
            best_score = score;
        }
    }
    if (!best) {
        error = "No real title was available for this recommendation.";
        return false;
    }
    result.item = *best;
    result.reason = "Chosen from your ReddMedia viewing history and real title metadata.";
    return true;
}

bool RecommendationEngine::recommend(const RecommendationRequest& request,
                                     const std::vector<MediaDescriptor>& local_items,
                                     RecommendationResult& result,
                                     std::string& error) {
    std::vector<MediaDescriptor> candidates;
    if (request.source == RecommendationSource::Local) {
        candidates = matching_local_items(request.media_type, local_items);
        if (candidates.empty()) {
            error = std::string("No Local ") + media_type_name(request.media_type) +
                " titles are linked to ReddMedia.";
            return false;
        }
    } else if (request.source == RecommendationSource::External) {
        if (!external_candidates(request, local_items, candidates, error)) return false;
    } else {
        error = "Live TV recommendations are handled by Nougat's tuner/EPG layer, not the TMDb recommendation engine.";
        return false;
    }

    candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
        [&request](const MediaDescriptor& item) {
            return item.media_type != request.media_type;
        }), candidates.end());
    if (candidates.empty()) {
        error = std::string("No ") + media_type_name(request.media_type) +
            " title matched this request.";
        return false;
    }

    if (request.mode == RecommendationMode::Random) {
        std::random_device device;
        std::mt19937 generator(device());
        std::uniform_int_distribution<std::size_t> distribution(0U, candidates.size() - 1U);
        result.item = candidates[distribution(generator)];
        result.reason = "Random choice; viewing history was not used.";
        if (result.item.media_type != request.media_type) {
            error = "ReddMedia blocked a mismatched recommendation type.";
            return false;
        }
        return true;
    }
    if (!usual_recommendation(request, candidates, result, error)) return false;
    if (result.item.media_type != request.media_type) {
        error = "ReddMedia blocked a mismatched recommendation type.";
        return false;
    }
    return true;
}

} // namespace reddmedia
