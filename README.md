# logos-irc-module

## How to Build

### Using Nix (Recommended)

#### Build Complete Module (Library + Headers)

```bash
# Build everything (default)
nix build

# Or explicitly
nix build '.#default'
```

The result will include:
- `/lib/logos_irc_plugin.dylib` (or `.so` on Linux) - The IRC module plugin
- `/include/` - Generated headers for the module API

#### Build Individual Components

```bash
# Build only the library (plugin)
nix build '.#lib'

# Build only the generated headers
nix build '.#logos-irc-module-include'
```

#### Build and Run Example Application

```bash
# Build the example application
nix build '.#example'

# Run the example application
nix run '.#example'
```

then connect with an IRC Client to localhost:6667

#### Development Shell

```bash
# Enter development shell with all dependencies
nix develop
```

**Note:** In zsh, you need to quote the target (e.g., `'.#default'`) to prevent glob expansion.

If you don't have flakes enabled globally, add experimental flags:

```bash
nix build --extra-experimental-features 'nix-command flakes'
```

The compiled artifacts can be found at `result/`

#### Nix Organization

The nix build system is organized into modular files in the `/nix` directory:
- `nix/default.nix` - Common configuration (dependencies, flags, metadata)
- `nix/lib.nix` - Module plugin compilation
- `nix/include.nix` - Header generation using logos-cpp-generator
- `nix/example.nix` - Example application build

## Output Structure

When built with Nix:

```
result/
├── lib/
│   └── logos_irc_plugin.dylib    # Logos IRC module plugin
├── include/
│   ├── logos_irc_api.h           # Generated SDK header
│   ├── logos_irc_api.cpp         # Generated SDK code
└── bin/
    └── logos-irc-example          # Example application
```

When running the example:

```
result/
├── bin/
│   ├── logos-irc-example         # Example application
│   ├── logoscore                 # Logos core binary
│   └── logos_host                # Logos host binary
├── lib/
│   ├── liblogos_core.dylib       # Core library
│   └── liblogos_sdk.a            # SDK library
└── modules/
    ├── capability_module_plugin.dylib
    ├── chat_plugin.dylib
    ├── logos_irc_plugin.dylib
    ├── waku_module_plugin.dylib
    └── libwaku.dylib
```

## Requirements

### Build Tools
- CMake (3.14 or later)
- Ninja build system
- pkg-config

### Dependencies
- Qt6 (qtbase)
- Qt6 Remote Objects (qtremoteobjects)
- logos-liblogos
- logos-cpp-sdk (for header generation)
- logos-waku-module
- logos-chat-module
- logos-capability-module
- zstd
- krb5
- abseil-cpp
