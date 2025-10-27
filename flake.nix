{
  description = "Logos IRC Module - An IRC plugin for Logos using Chat";

  inputs = {
    # Follow the same nixpkgs as logos-liblogos to ensure compatibility
    nixpkgs.follows = "logos-liblogos/nixpkgs";
    logos-cpp-sdk.url = "github:logos-co/logos-cpp-sdk";
    logos-liblogos.url = "github:logos-co/logos-liblogos";
    logos-chat-module.url = "github:logos-co/logos-chat-module";
    logos-waku-module.url = "github:logos-co/logos-waku-module";
    logos-capability-module.url = "github:logos-co/logos-capability-module";
  };

  outputs = { self, nixpkgs, logos-cpp-sdk, logos-liblogos, logos-chat-module, logos-waku-module, logos-capability-module }:
    let
      systems = [ "aarch64-darwin" "x86_64-darwin" "aarch64-linux" "x86_64-linux" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f {
        pkgs = import nixpkgs { inherit system; };
        logosSdk = logos-cpp-sdk.packages.${system}.default;
        logosLiblogos = logos-liblogos.packages.${system}.default;
        logosChatModule = logos-chat-module.packages.${system}.default;
        logosWakuModule = logos-waku-module.packages.${system}.default;
        logosCapabilityModule = logos-capability-module.packages.${system}.default;
      });
    in
    {
      packages = forAllSystems ({ pkgs, logosSdk, logosLiblogos, logosChatModule, logosWakuModule, logosCapabilityModule }: 
        let
          # Common configuration
          common = import ./nix/default.nix { 
            inherit pkgs logosSdk logosLiblogos; 
          };
          src = ./.;
          
          # Library package
          lib = import ./nix/lib.nix { 
            inherit pkgs common src logosChatModule logosSdk; 
          };

          # Include package (generated headers from plugin)
          include = import ./nix/include.nix {
            inherit pkgs common src lib logosSdk;
          };

          # Example package
          example = import ./nix/example.nix {
            inherit pkgs common src logosSdk logosLiblogos logosWakuModule logosChatModule logosCapabilityModule;
            logosIrcModuleLib = lib;
          };

          # Combined package
          combined = pkgs.symlinkJoin {
            name = "logos-irc-module";
            paths = [ lib include ];
          };
        in
        {
          # Individual outputs
          logos-irc-module-lib = lib;
          logos-irc-module-include = include;
          example = example;
          
          # Default package (combined)
          default = combined;
        }
      );

      devShells = forAllSystems ({ pkgs, logosSdk, logosLiblogos, logosChatModule, logosWakuModule, logosCapabilityModule }: {
        default = pkgs.mkShell {
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
            pkgs.abseil-cpp
          ];
          
          shellHook = ''
            export LOGOS_CPP_SDK_ROOT="${logosSdk}"
            export LOGOS_LIBLOGOS_ROOT="${logosLiblogos}"
            echo "Logos IRC Module development environment"
            echo "LOGOS_CPP_SDK_ROOT: $LOGOS_CPP_SDK_ROOT"
            echo "LOGOS_LIBLOGOS_ROOT: $LOGOS_LIBLOGOS_ROOT"
          '';
        };
      });
    };
}
