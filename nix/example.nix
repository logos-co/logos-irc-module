# Builds the logos-irc-module example application
{ pkgs, common, src, logosSdk, logosLiblogos, logosWakuModule, logosChatModule, logosCapabilityModule, logosIrcModuleLib }:

pkgs.stdenv.mkDerivation rec {
  pname = "logos-irc-example";
  version = common.version;
  
  inherit src;
  
  # This is an aggregate runtime layout; avoid stripping to prevent hook errors
  dontStrip = true;
  
  # This is a CLI application, disable Qt wrapping
  dontWrapQtApps = true;
  
  nativeBuildInputs = [ 
    pkgs.cmake 
    pkgs.ninja 
    pkgs.pkg-config
  ];
  
  buildInputs = [ 
    pkgs.qt6.qtbase
    pkgs.qt6.qtremoteobjects
    pkgs.zstd
    pkgs.krb5
    logosLiblogos
    logosSdk
    logosWakuModule
    logosChatModule
    logosCapabilityModule
  ];
  
  # Configure and build phase
  configurePhase = ''
    runHook preConfigure
    
    echo "Configuring logos-irc-example..."
    echo "liblogos: ${logosLiblogos}"
    echo "cpp-sdk: ${logosSdk}"
    echo "waku-module: ${logosWakuModule}"
    echo "chat-module: ${logosChatModule}"
    echo "capability-module: ${logosCapabilityModule}"
    
    # Verify that the built components exist
    test -d "${logosLiblogos}" || (echo "liblogos not found" && exit 1)
    test -d "${logosSdk}" || (echo "cpp-sdk not found" && exit 1)
    test -d "${logosWakuModule}" || (echo "waku-module not found" && exit 1)
    test -d "${logosChatModule}" || (echo "chat-module not found" && exit 1)
    test -d "${logosCapabilityModule}" || (echo "capability-module not found" && exit 1)
    
    # Build the example application
    echo "Building example application..."
    cmake -S example -B build \
      -GNinja \
      -DCMAKE_BUILD_TYPE=Release \
      -DLOGOS_LIBLOGOS_ROOT=${logosLiblogos} \
      -DLOGOS_CPP_SDK_ROOT=${logosSdk}
    
    runHook postConfigure
  '';
  
  buildPhase = ''
    runHook preBuild
    
    cmake --build build
    echo "logos-irc-example built successfully!"
    
    runHook postBuild
  '';
  
  installPhase = ''
    runHook preInstall
    
    # Create output directories
    mkdir -p $out/bin $out/lib $out/modules
    
    # Install our example binary
    if [ -f "build/bin/logos-irc-example" ]; then
      cp build/bin/logos-irc-example "$out/bin/"
      echo "Installed logos-irc-example binary"
    fi
    
    # Copy the core binaries from liblogos
    if [ -f "${logosLiblogos}/bin/logoscore" ]; then
      cp -L "${logosLiblogos}/bin/logoscore" "$out/bin/"
      echo "Installed logoscore binary"
    fi
    if [ -f "${logosLiblogos}/bin/logos_host" ]; then
      cp -L "${logosLiblogos}/bin/logos_host" "$out/bin/"
      echo "Installed logos_host binary"
    fi
    
    # Copy required shared libraries from liblogos
    if ls "${logosLiblogos}/lib/"liblogos_core.* >/dev/null 2>&1; then
      cp -L "${logosLiblogos}/lib/"liblogos_core.* "$out/lib/" || true
    fi
    
    # Copy SDK library if it exists
    if ls "${logosSdk}/lib/"liblogos_sdk.* >/dev/null 2>&1; then
      cp -L "${logosSdk}/lib/"liblogos_sdk.* "$out/lib/" || true
    fi

    # Determine platform-specific plugin extension
    OS_EXT="so"
    case "$(uname -s)" in
      Darwin) OS_EXT="dylib";;
      Linux) OS_EXT="so";;
      MINGW*|MSYS*|CYGWIN*) OS_EXT="dll";;
    esac

    # Copy module plugins into the modules directory
    if [ -f "${logosCapabilityModule}/lib/capability_module_plugin.$OS_EXT" ]; then
      cp -L "${logosCapabilityModule}/lib/capability_module_plugin.$OS_EXT" "$out/modules/"
    fi
    if [ -f "${logosWakuModule}/lib/waku_module_plugin.$OS_EXT" ]; then
      cp -L "${logosWakuModule}/lib/waku_module_plugin.$OS_EXT" "$out/modules/"
    fi
    if [ -f "${logosChatModule}/lib/chat_plugin.$OS_EXT" ]; then
      cp -L "${logosChatModule}/lib/chat_plugin.$OS_EXT" "$out/modules/"
    fi
    
    # Copy libwaku library to modules directory (needed by waku_module_plugin)
    if [ -f "${logosWakuModule}/lib/libwaku.$OS_EXT" ]; then
      cp -L "${logosWakuModule}/lib/libwaku.$OS_EXT" "$out/modules/"
    fi

    # Copy the IRC module plugin (from pre-built lib)
    if [ -f "${logosIrcModuleLib}/lib/logos_irc_plugin.$OS_EXT" ]; then
      cp -L "${logosIrcModuleLib}/lib/logos_irc_plugin.$OS_EXT" "$out/modules/"
    fi

    # Create a README for reference
    cat > $out/README.txt <<EOF
Logos IRC Module Example - Build Information
===========================================
liblogos: ${logosLiblogos}
cpp-sdk: ${logosSdk}
waku-module: ${logosWakuModule}
chat-module: ${logosChatModule}
capability-module: ${logosCapabilityModule}

Runtime Layout:
- Binaries: $out/bin
- Libraries: $out/lib
- Modules: $out/modules

Usage:
  $out/bin/logos-irc-example --module-path $out/modules
EOF
    
    runHook postInstall
  '';
  
  meta = with pkgs.lib; {
    description = "Logos IRC Module Example - Demonstrates loading IRC module with dependencies";
    platforms = platforms.unix;
  };
}
