# file-server-cli

CLI version of https://github.com/Sciencewolf/file-server-raspberry-pi.

## Setup

- Configure the project:

```sh
cmake -S . -B build -G Ninja   -DCMAKE_BUILD_TYPE=Release   -DCMAKE_C_COMPILER=gcc-14   -DCMAKE_CXX_COMPILER=g++-14
```

- Build the project:

```sh
cmake --build build
```

## Usage

```sh
fscli get
```

Get all files from the server.

```sh
fscli get [filename_number]
```

Get a specific file by the index from the `/get` list.
