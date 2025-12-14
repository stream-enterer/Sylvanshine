#include "animation_player.hpp"

AnimationPlayer::AnimationPlayer()
    : atlas_(nullptr),
      current_animation_(nullptr),
      current_frame_index_(0),
      elapsed_time_(0.0f),
      frame_delay_(DEFAULT_FRAME_DELAY),
      is_playing_(false),
      is_looping_(false),
      is_finished_(false) {}

void AnimationPlayer::SetAtlas(TextureAtlas* atlas) {
    atlas_ = atlas;
}

bool AnimationPlayer::Play(const std::string& animation_name, bool loop) {
    if (!atlas_) return false;

    const Animation* anim = atlas_->GetAnimation(animation_name);
    if (!anim) return false;

    current_animation_ = anim;
    current_animation_name_ = animation_name;
    current_frame_index_ = 0;
    elapsed_time_ = 0.0f;
    is_playing_ = true;
    is_looping_ = loop;
    is_finished_ = false;

    if (anim->fps > 0) {
        frame_delay_ = 1.0f / static_cast<float>(anim->fps);
    }

    return true;
}

void AnimationPlayer::Update(float dt) {
    if (!is_playing_ || !current_animation_ || current_animation_->frames.empty()) {
        return;
    }

    elapsed_time_ += dt;

    int frame_count = static_cast<int>(current_animation_->frames.size());
    int target_frame = static_cast<int>(elapsed_time_ / frame_delay_);

    if (is_looping_) {
        current_frame_index_ = target_frame % frame_count;
    } else {
        if (target_frame >= frame_count) {
            current_frame_index_ = frame_count - 1;
            is_finished_ = true;
            is_playing_ = false;
        } else {
            current_frame_index_ = target_frame;
        }
    }
}

void AnimationPlayer::Stop() {
    is_playing_ = false;
    current_frame_index_ = 0;
    elapsed_time_ = 0.0f;
}

void AnimationPlayer::SetFrameDelay(float delay) {
    frame_delay_ = delay;
}

const AnimationFrame* AnimationPlayer::GetCurrentFrame() const {
    if (!current_animation_ || current_animation_->frames.empty()) {
        return nullptr;
    }

    if (current_frame_index_ >= 0 &&
        current_frame_index_ < static_cast<int>(current_animation_->frames.size())) {
        return &current_animation_->frames[current_frame_index_];
    }

    return nullptr;
}

float AnimationPlayer::GetProgress() const {
    if (!current_animation_ || current_animation_->frames.empty()) {
        return 0.0f;
    }

    float total_duration = frame_delay_ * static_cast<float>(current_animation_->frames.size());
    return elapsed_time_ / total_duration;
}
