#!/usr/bin/env fish
set BUILD_DIR "build"
set PROJECT_NAME "tactics"
set SHADER_SRC_DIR "shaders"
set SHADER_OUT_DIR "dist/shaders"
set DIST_DIR "dist"

function check_glslangValidator
    if not command -v glslangValidator &>/dev/null
        echo "✗ glslangValidator not found."
        echo "Install: emerge dev-util/glslang"
        return 1
    end
end

function do_compile_shaders
    echo "→ Compiling shaders..."
    check_glslangValidator; or return 1

    if not test -d "$SHADER_SRC_DIR"
        echo "✗ Shader source directory not found: $SHADER_SRC_DIR"
        return 1
    end

    mkdir -p "$SHADER_OUT_DIR"

    set -l compiled 0
    set -l skipped 0
    for shader in "$SHADER_SRC_DIR"/*.vert "$SHADER_SRC_DIR"/*.frag
        test -f "$shader"; or continue
        set -l name (basename "$shader")
        set -l out "$SHADER_OUT_DIR/$name.spv"

        # Skip if output exists and is newer than source
        if test -f "$out" -a "$out" -nt "$shader"
            set skipped (math $skipped + 1)
            continue
        end

        if not glslangValidator -V "$shader" -o "$out" >/dev/null 2>&1
            echo "  ✗ Failed to compile $name"
            glslangValidator -V "$shader" -o "$out"
            return 1
        end
        echo "  Compiled $name"
        set compiled (math $compiled + 1)
    end

    if test $compiled -gt 0
        echo "  Compiled $compiled shaders"
    end
    if test $skipped -gt 0
        echo "  Skipped $skipped unchanged shaders"
    end
end

function check_assets_stale
    # Returns 0 if assets need rebuilding, 1 if up-to-date
    set -l manifest "dist/assets.json"

    # If manifest doesn't exist, need to build
    if not test -f "$manifest"
        return 0
    end

    # Check if any source file is newer than manifest
    # Use find for reliable recursive globbing
    for src in (find app/resources -name "*.png" -o -name "*.plist" 2>/dev/null)
        if test "$src" -nt "$manifest"
            return 0
        end
    end
    for src in app/timing/*.tsv app/fx/*.tsv
        test -f "$src"; or continue
        if test "$src" -nt "$manifest"
            return 0
        end
    end

    # All sources older than manifest
    return 1
end

function do_build_assets
    echo "→ Building assets..."
    # Early exit if nothing changed
    if not check_assets_stale
        echo "  Assets up-to-date (skipping Python)"
        return 0
    end
    uv run build_assets.py; or return 1
end

function do_build
    do_build_assets or return 1
    do_compile_shaders or return 1

    echo "→ Configuring release build..."
    cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release
    and echo "→ Building with "(nproc)" cores..."
    and cmake --build $BUILD_DIR -j(nproc)
    and echo "✓ Build complete: ./$BUILD_DIR/$PROJECT_NAME"
end

function do_debug
    do_build_assets or return 1
    do_compile_shaders or return 1

    echo "→ Configuring debug build..."
    cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Debug
    and echo "→ Building with "(nproc)" cores..."
    and cmake --build $BUILD_DIR -j(nproc)
    and echo "✓ Debug build complete: ./$BUILD_DIR/$PROJECT_NAME"
end

function do_clean
    echo "→ Cleaning build directory..."
    rm -rf $BUILD_DIR
    and echo "✓ Removed $BUILD_DIR/"
end

function do_rebuild
    do_clean
    and do_build
end

function do_run
    if test -f ./$BUILD_DIR/$PROJECT_NAME
        echo "→ Running $PROJECT_NAME..."
        ./$BUILD_DIR/$PROJECT_NAME $argv
    else
        echo "✗ Executable not found. Run './build.fish build' first."
        return 1
    end
end

function do_build_run
    do_build
    and do_run $argv
end

function do_build_run_fullscreen
    do_build
    and do_run -f $argv
end

function do_quick
    # Quick build: skip assets and shaders, just compile C++
    echo "→ Quick build (C++ only)..."
    cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release
    and cmake --build $BUILD_DIR -j(nproc)
    and echo "✓ Quick build complete: ./$BUILD_DIR/$PROJECT_NAME"
end

function do_quick_run
    do_quick
    and do_run $argv
end

function do_quick_run_fullscreen
    do_quick
    and do_run -f $argv
end

function do_shaders_only
    do_compile_shaders
end

# Disabled: wipes dist/ which is slow to regenerate
# function do_assets_only
#     echo "→ Building assets (forced)..."
#     uv run build_assets.py --clean; or return 1
#     echo "✓ Assets built to $DIST_DIR/"
# end

function show_help
    echo "Usage: ./build.fish [command] [args...]"
    echo ""
    echo "Commands:"
    echo "  build      - Build assets + shaders + release"
    echo "  debug      - Build assets + shaders + debug"
    echo "  quick      - C++ only (skip assets/shaders)"
    echo "  qr         - Quick + run"
    echo "  qrf        - Quick + run fullscreen"
    echo "  clean      - Remove build directory"
    echo "  rebuild    - Clean + build"
    echo "  run        - Run executable"
    echo "  runf       - Run fullscreen"
    echo "  br         - Build + run"
    echo "  brf        - Build + run fullscreen"
    echo "  shaders    - Compile shaders only"
    echo "  help       - Show this help"
    echo ""
    echo "Args passed to executable (e.g., './build.fish br -f' for fullscreen)"
end

switch $argv[1]
    case build
        do_build
    case debug
        do_debug
    case quick
        do_quick
    case qr
        do_quick_run $argv[2..]
    case qrf
        do_quick_run_fullscreen $argv[2..]
    case clean
        do_clean
    case rebuild
        do_rebuild
    case run
        do_run $argv[2..]
    case runf
        do_run -f $argv[2..]
    case br
        do_build_run $argv[2..]
    case brf
        do_build_run_fullscreen $argv[2..]
    case shaders
        do_shaders_only
    # case assets
    #     do_assets_only
    case help
        show_help
    case '*'
        if test (count $argv) -eq 0
            do_build
        else
            echo "Unknown command: $argv[1]"
            show_help
            return 1
        end
end
