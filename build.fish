#!/usr/bin/env fish

set BUILD_DIR "build"
set PROJECT_NAME "tactics"

function do_build
    echo "→ Configuring release build..."
    cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release
    and echo "→ Building with "(nproc)" cores..."
    and cmake --build $BUILD_DIR -j(nproc)
    and echo "✓ Build complete: ./$BUILD_DIR/$PROJECT_NAME"
end

function do_debug
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

function show_help
    echo "Usage: ./build.fish [command]"
    echo ""
    echo "Commands:"
    echo "  build      - Build release"
    echo "  debug      - Build debug"
    echo "  clean      - Remove build directory"
    echo "  rebuild    - Clean + build"
    echo "  run        - Run executable"
    echo "  br         - Build + run"
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
    case help
        show_help
    case '*'
        if test (count $argv) -eq 0
            do_build
        else
            echo "Unknown command: $argv[1]"
            show_help
            exit 1
        end
end
