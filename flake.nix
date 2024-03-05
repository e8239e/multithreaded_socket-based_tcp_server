{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    flake-compat.url = "github:edolstra/flake-compat";
    flake-compat.flake = false;
  };

  outputs = inputs@{ self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        writeRubyApplication =
          { name
          , text
          , ruby ? pkgs.ruby
          , jit ? (nixpkgs.lib.lists.elem "--enable-yjit" ruby.configureFlags)
          , gems ? [ ]
          , runtimeInputs ? [ ]
          , meta ? { }
          , checkPhase ? null
          }:
          let
            rubyEnv = if (gems == [ ]) then ruby else (ruby.withPackages (p: with p; gems));
          in
          pkgs.writeTextFile {
            inherit name meta;
            executable = true;
            destination = "/bin/${name}";
            allowSubstitutes = true;
            preferLocalBuild = false;
            text = ''
              #!${nixpkgs.lib.getExe' rubyEnv "ruby"} ${nixpkgs.lib.optionalString jit "--yjit"}

              # frozen_string_literal: true

            ''
            + nixpkgs.lib.optionalString (runtimeInputs != [ ]) ''

              ENV['PATH'] = '${nixpkgs.lib.makeBinPath runtimeInputs}:' + ENV['PATH']

            '' +
            ''

            '' + "\n${text}\n";

            checkPhase =
              let
                rubySupported = nixpkgs.lib.meta.availableOn pkgs.stdenv.buildPlatform ruby;
                rubycheckCommand = nixpkgs.lib.optionalString rubySupported ''
                  ${ruby}/bin/ruby -c "$target"
                '';
              in
              if checkPhase == null then ''
                runHook preCheck
                ${rubycheckCommand}
                runHook postCheck
              ''
              else checkPhase;
          };
      in
      rec {
        formatter = pkgs.nixpkgs-fmt;
        devShells.default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            formatter
            valgrind # test for memory errors & leaks
            bear # extract compile commands for lsp
            clang-tools # lsp (clangd)
            ccls # lsp
            (writeRubyApplication {
              name = "get_clang-format_config";
              ruby = pkgs.ruby_3_2;
              text =
                ''
                  require 'yaml'
                  require 'open-uri'

                  resp = URI.open 'https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/plain/.clang-format'
                  exit 1 if resp.status[0].to_i != 200
                  resp = YAML.safe_load resp.read

                  resp.filter! { |k, _| k != 'ForEachMacros' }

                  { # customizations
                    'SortIncludes' => true,
                    'AlwaysBreakTemplateDeclarations' => true,
                    'AccessModifierOffset' => 0,
                    'AlignArrayOfStructures' => 'Right',
                    'PackConstructorInitializers' => 'Never',
                    'PointerAlignment' => 'Left',
                    'SpaceBeforeCpp11BracedList' => false,
                    'Standard' => 'c++20',
                    'Language' => 'Cpp',
                    'NamespaceIndentation' => 'All',
                    'ShortNamespaceLines' => 0,
                    'FixNamespaceComments' => true
                  }.each_pair { |k, v| resp[k] = v }

                  {
                    'AfterFunction' => false,
                    'BeforeCatch' => true,
                    'BeforeElse' => true,
                    'BeforeLambdaBody' => true
                  }.each_pair { |k, v| resp['BraceWrapping'][k] = v }

                  resp['BraceWrapping']['AfterFunction'] = false

                  puts resp.to_yaml
                  warn 'done'
                '';
            })
          ];
          buildInputs = with pkgs; [ ];
        };
      });
}
