# file-server-cli

CLI version of https://github.com/Sciencewolf/file-server-raspberry-pi.

`fscli` manages files on https://files.martonaron.dev from the terminal.

## Setup

Configure the project:

```sh
cmake -S . -B build -G Ninja
```

Raspberry Pi:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc-14 -DCMAKE_CXX_COMPILER=g++-14
```

Build the project:

```sh
cmake --build build
```

The executable is written to:

```sh
build/fscli
```

## Usage

```sh
fscli <option>
```

## Commands

| Command | Alias | Description |
| --- | --- | --- |
| `ls` | `-l` | List files on the server. |
| `get` | `-g` | With no index, list files. With an index, download that file to your Downloads folder. |
| `up <path>` | `-u <path>` | Upload a local file. |
| `del` | `-d` | With no index, list files. With an index, delete that file from the server. |
| `rn` | `-r` | With no index, list files. With an index and new name, rename that file while keeping its existing extension. |
| `prev <index>` | `-p <index>` | Print the web preview URL for a file. |
| `words` | `-w` | Print available command keywords. |

## Examples

```sh
fscli ls
fscli get 1
fscli up ./example.pdf
fscli rn 1 new_file_name
fscli del 1
fscli prev 1
fscli words
```

Invalid commands or invalid file indexes return a non-zero exit code and print an error message.

Renaming keeps the old extension when the original file has one. Files without an extension keep the exact new name.
