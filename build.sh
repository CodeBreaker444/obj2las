# add a clean function method to the build script

function clean() {
    echo "Cleaning build directory"
    rm -rf build
    echo "Clean complete"
}
if [ "$1" == "build" ]; then
    clean
    echo "Creating build directory"
    mkdir -p build
    cd build
    echo "Building project"
    cmake ..
    make
    echo "Build complete"
elif [ "$1" == "test" ]; then
    echo "Creating build directory"
    mkdir -p build
    cd build
    echo "Building project"
    cmake ..
    make test
    echo "Build complete"
    echo "Running tests"
    ctest
    echo "Tests complete"
elif [ "$1" == "clean" ]; then
    clean
else
    echo "Invalid argument. Please use 'build' or 'test'"
fi

