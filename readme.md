```
       _     _ ____  _
  ___ | |__ (_)___ \| | __ _ ___
 / _ \| '_ \| | __) | |/ _` / __|
| (_) | |_) | |/ __/| | (_| \__ \
 \___/|_.__// |_____|_|\__,_|___/
          |__/            v1.0.0a
        << OBJ to LAS Converter >>
```

# OBJ to LAS Converter

Convert 3D OBJ files to LAS point cloud format with support for materials, textures, and coordinate transformations.

``` Start D3D Platform ```
> Update: OBJ to 3d Tiles Converter is also available at : https://app.d3d.ai/free/converter/objto3dtile
> <br>Supports Large models upto 100 GB (available for enterprise), Preserves high-texture quality.
#### OBJ to 3D Tiles

<p align="center">
  <img src="https://github.com/CodeBreaker444/obj2las/blob/main/deepinspect3d.gif?raw=true" alt="DeepInspect 3d" width="720" heigh="480"/>
</p>
<p align="center">
Online platform to render 3d models: 
<a href="https:d3d.ai" align="center">DeepInspect 3d</a>
</p>

``` End D3D Platform```
 
## Features

- Converts OBJ files to LAS-1.3 format
- Supports MTL materials and multiple textures
- Samples colors from textures and converts to vertex colors
- Automatic global to local coordinate translation
- Built with C++11

## Prerequisites

- CMake (3.10 or higher)
- C++11 compatible compiler (g++)
- Linux/Unix environment

## Building

1. Clone the repository:
```bash
git clone https://github.com/codebreaker444/obj2las.git
cd obj2las
```

2. Build using the provided script:
```bash
chmod +x build.sh
./build.sh build
```

## Usage

```bash
./build/obj2las <input.obj> <output.las>
```

Example:
```bash
./build/obj2las model.obj output.las
```

## Running Tests

Run simple test with capsule model:
```bash
./build.sh test
```

Run complex test with detailed model:
```bash
./build.sh test_complex
```

Run test with coordinate shifting:
```bash
./build.sh test_complex_shift
```

## Cleaning Build Files

```bash
./build.sh clean
```

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License
Copyright 2024

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.

You may obtain a copy of the License at
```
http://www.apache.org/licenses/LICENSE-2.0
```


Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
