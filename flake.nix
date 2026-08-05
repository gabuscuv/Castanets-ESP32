{
  description = "ESP32 development shell using nixpkgs-esp-dev.";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
    nixpkgs-esp-dev = {
      url = "github:mirrexagon/nixpkgs-esp-dev";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, nixpkgs-esp-dev }: {
    devShells.x86_64-linux.default = let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
        overlays = [ (import "${nixpkgs-esp-dev}/overlay.nix") ];
        config = {
          permittedInsecurePackages = [ "python3.13-ecdsa-0.19.1" "python3.13-esp-coredump-1.13.1" ];
        };
      };
    in pkgs.mkShell {
      name = "esp-project";
      #inputsFrom = [ pkgs.esp-idf-full ];
      buildInputs = [ pkgs.esp-idf-full pkgs.qemu-esp32 ];
      shellHook =''
        export PATH="${pkgs.qemu-esp32}/bin:$PATH"
        export ESP_IDF_PATH="${pkgs.esp-idf-full}"
      '';
    };
  };
}
