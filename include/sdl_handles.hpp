#pragma once
#include <SDL3/SDL.h>

struct TextureHandle {
    SDL_Texture* ptr = nullptr;

    TextureHandle() = default;
    explicit TextureHandle(SDL_Texture* p) : ptr(p) {}

    ~TextureHandle() {
        if (ptr) SDL_DestroyTexture(ptr);
    }

    TextureHandle(TextureHandle&& o) noexcept : ptr(o.ptr) {
        o.ptr = nullptr;
    }

    TextureHandle& operator=(TextureHandle&& o) noexcept {
        if (this != &o) {
            if (ptr) SDL_DestroyTexture(ptr);
            ptr = o.ptr;
            o.ptr = nullptr;
        }
        return *this;
    }

    TextureHandle(const TextureHandle&) = delete;
    TextureHandle& operator=(const TextureHandle&) = delete;

    SDL_Texture* get() const { return ptr; }
    explicit operator bool() const { return ptr != nullptr; }
};

struct SurfaceHandle {
    SDL_Surface* ptr = nullptr;

    SurfaceHandle() = default;
    explicit SurfaceHandle(SDL_Surface* p) : ptr(p) {}

    ~SurfaceHandle() {
        if (ptr) SDL_DestroySurface(ptr);
    }

    SurfaceHandle(SurfaceHandle&& o) noexcept : ptr(o.ptr) {
        o.ptr = nullptr;
    }

    SurfaceHandle& operator=(SurfaceHandle&& o) noexcept {
        if (this != &o) {
            if (ptr) SDL_DestroySurface(ptr);
            ptr = o.ptr;
            o.ptr = nullptr;
        }
        return *this;
    }

    SurfaceHandle(const SurfaceHandle&) = delete;
    SurfaceHandle& operator=(const SurfaceHandle&) = delete;

    SDL_Surface* get() const { return ptr; }
    explicit operator bool() const { return ptr != nullptr; }
};

struct RendererHandle {
    SDL_Renderer* ptr = nullptr;

    RendererHandle() = default;
    explicit RendererHandle(SDL_Renderer* p) : ptr(p) {}

    ~RendererHandle() {
        if (ptr) SDL_DestroyRenderer(ptr);
    }

    RendererHandle(RendererHandle&& o) noexcept : ptr(o.ptr) {
        o.ptr = nullptr;
    }

    RendererHandle& operator=(RendererHandle&& o) noexcept {
        if (this != &o) {
            if (ptr) SDL_DestroyRenderer(ptr);
            ptr = o.ptr;
            o.ptr = nullptr;
        }
        return *this;
    }

    RendererHandle(const RendererHandle&) = delete;
    RendererHandle& operator=(const RendererHandle&) = delete;

    SDL_Renderer* get() const { return ptr; }
    explicit operator bool() const { return ptr != nullptr; }
};

struct WindowHandle {
    SDL_Window* ptr = nullptr;

    WindowHandle() = default;
    explicit WindowHandle(SDL_Window* p) : ptr(p) {}

    ~WindowHandle() {
        if (ptr) SDL_DestroyWindow(ptr);
    }

    WindowHandle(WindowHandle&& o) noexcept : ptr(o.ptr) {
        o.ptr = nullptr;
    }

    WindowHandle& operator=(WindowHandle&& o) noexcept {
        if (this != &o) {
            if (ptr) SDL_DestroyWindow(ptr);
            ptr = o.ptr;
            o.ptr = nullptr;
        }
        return *this;
    }

    WindowHandle(const WindowHandle&) = delete;
    WindowHandle& operator=(const WindowHandle&) = delete;

    SDL_Window* get() const { return ptr; }
    explicit operator bool() const { return ptr != nullptr; }
};
