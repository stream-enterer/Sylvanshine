#!/usr/bin/env fish
set BUILD_DIR "build"
set PROJECT_NAME "tactics"
set DUELYST_REPO "$HOME/.local/git/duelyst"
set SHADER_DIR "data/shaders"
set GLSL_DIR "$SHADER_DIR/glsl"
set SPIRV_OUT_DIR "$SHADER_DIR/compiled/spirv"

function check_glslangValidator
    if not command -v glslangValidator &>/dev/null
        echo "✗ glslangValidator not found."
        echo "Install: emerge dev-util/glslang"
        return 1
    end
end

function do_compile_shaders
    echo "→ Compiling shaders..."
    check_glslangValidator or return 1
    
    if not test -d "$GLSL_DIR"
        echo "✗ GLSL directory not found: $GLSL_DIR"
        return 1
    end
    
    mkdir -p "$SPIRV_OUT_DIR"
    
    set -l count 0
    for shader in "$GLSL_DIR"/*.vert "$GLSL_DIR"/*.frag
        test -f "$shader" or continue
        set -l name (basename "$shader")
        set -l out "$SPIRV_OUT_DIR/$name.spv"
        echo "  Compiling $name → $out"
        glslangValidator -V "$shader" -o "$out" or return 1
        set count (math $count + 1)
    end
    
    echo "✓ Compiled $count shaders to $SPIRV_OUT_DIR/"
end

function do_build
    do_compile_shaders or return 1
    
    echo "→ Configuring release build..."
    cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release -DDUELYST_REPO_PATH=$DUELYST_REPO
    and echo "→ Building with "(nproc)" cores..."
    and cmake --build $BUILD_DIR -j(nproc)
    and echo "✓ Build complete: ./$BUILD_DIR/$PROJECT_NAME"
end

function do_debug
    do_compile_shaders or return 1
    
    echo "→ Configuring debug build..."
    cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Debug -DDUELYST_REPO_PATH=$DUELYST_REPO
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

function do_shaders_only
    do_compile_shaders
end

function show_help
    echo "Usage: ./build.fish [command]"
    echo ""
    echo "Commands:"
    echo "  build      - Compile shaders + build release"
    echo "  debug      - Compile shaders + build debug"
    echo "  clean      - Remove build directory"
    echo "  rebuild    - Clean + build"
    echo "  run        - Run executable"
    echo "  br         - Build + run"
    echo "  shaders    - Compile shaders only"
    echo "  help       - Show this help"
end

switch $argv[1]
    case build
        do_build
    case debug
        do_debug
    case clean
        do_clean
    case rebuild
        do_rebuild
    case run
        do_run $argv[2..]
    case br
        do_build_run $argv[2..]
    case shaders
        do_shaders_only
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
