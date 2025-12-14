#ifndef ANIMATION_PLAYER_HPP
#define ANIMATION_PLAYER_HPP

#include "texture_atlas.hpp"
#include "types.hpp"

#include <string>

class AnimationPlayer {
public:
    AnimationPlayer();

    void SetAtlas(TextureAtlas* atlas);
    bool Play(const std::string& animation_name, bool loop = false);
    void Update(float dt);
    void Stop();

    void SetFrameDelay(float delay);

    const AnimationFrame* GetCurrentFrame() const;
    bool IsPlaying() const { return is_playing_; }
    bool IsFinished() const { return is_finished_; }
    std::string GetCurrentAnimationName() const { return current_animation_name_; }
    float GetProgress() const;

private:
    TextureAtlas* atlas_;
    const Animation* current_animation_;
    std::string current_animation_name_;

    int current_frame_index_;
    float elapsed_time_;
    float frame_delay_;
    bool is_playing_;
    bool is_looping_;
    bool is_finished_;
};

#endif
