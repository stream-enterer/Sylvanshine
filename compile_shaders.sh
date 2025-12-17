#!/bin/bash
set -e

SHADER_DIR="${1:-data/shaders}"
GLSL_DIR="$SHADER_DIR/glsl"
OUT_DIR="$SHADER_DIR/compiled/spirv"

if ! command -v glslangValidator &>/dev/null; then
    echo "Error: glslangValidator not found."
    echo "Install: emerge dev-util/glslang"
    exit 1
fi

mkdir -p "$OUT_DIR"

if [[ ! -d "$GLSL_DIR" ]]; then
    echo "Error: GLSL directory not found: $GLSL_DIR"
    exit 1
fi

count=0
for shader in "$GLSL_DIR"/*.vert "$GLSL_DIR"/*.frag; do
    [[ -f "$shader" ]] || continue
    name=$(basename "$shader")
    out="$OUT_DIR/${name}.spv"
    echo "Compiling $name -> $out"
    glslangValidator -V "$shader" -o "$out"
    ((count++))
done

echo "Compiled $count shaders to $OUT_DIR/"
