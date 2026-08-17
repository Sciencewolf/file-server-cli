# file-server-cli

CLI version of https://github.com/Sciencewolf/file-server-raspberry-pi.

## Setup

- Configure the project:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
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
