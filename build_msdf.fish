#!/usr/bin/env fish

# Build msdf-atlas-gen without Skia, using system freetype/libpng

set -l repo_root (dirname (status filename))
set -l build_dir $repo_root/third_party/msdf-atlas-gen/build

echo "Configuring msdf-atlas-gen (no Skia, system libs)..."
cmake -S $repo_root/third_party/msdf-atlas-gen \
      -B $build_dir \
      -DCMAKE_BUILD_TYPE=Release \
      -DMSDF_ATLAS_USE_VCPKG=OFF \
      -DMSDF_ATLAS_USE_SKIA=OFF \
      -DMSDF_ATLAS_BUILD_STANDALONE=ON \
      -DMSDF_ATLAS_NO_ARTERY_FONT=ON

if test $status -ne 0
    echo "CMake configure failed"
    exit 1
end

echo "Building..."
cmake --build $build_dir --config Release -j(nproc)

if test $status -ne 0
    echo "Build failed"
    exit 1
end

echo "Done! Binary at:"
echo "  $build_dir/bin/msdf-atlas-gen"

# Optionally symlink to project bin
mkdir -p $repo_root/bin
ln -sf $build_dir/bin/msdf-atlas-gen $repo_root/bin/msdf-atlas-gen
echo "Symlinked to bin/msdf-atlas-gen"
