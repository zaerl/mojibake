# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.MixProject do
  use Mix.Project

  @version "0.8.0"
  @source_url "https://github.com/zaerl/mojibake"

  # Precompiled NIFs are assets of the GitHub release of each version, built by
  # .github/workflows/elixir-precompile.yml. Every other target compiles from source. A NIF built
  # for NIF version 2.16 (OTP 24 and 25) also loads on every later OTP.
  @precompiled_url "#{@source_url}/releases/download/v#{@version}/@{artefact_filename}"
  @precompiled_targets %{
    {:unix, :linux} => %{
      "x86_64-linux-gnu" => "x86_64-linux-gnu-",
      "aarch64-linux-gnu" => "aarch64-linux-gnu-"
    },
    {:unix, :darwin} => %{
      "x86_64-apple-darwin" => {"cc", "cc", "<%= cc %> -arch x86_64", "<%= cxx %> -arch x86_64"},
      "aarch64-apple-darwin" => {"cc", "cc", "<%= cc %> -arch arm64", "<%= cxx %> -arch arm64"}
    },
    {:win32, :nt} => %{
      "x86_64-windows-msvc" => {"cl", "cl"}
    }
  }
  @features [:names, :collation, :idna, :security]
  @feature_defines %{
    names: "MJB_FEATURE_CHARACTER_NAMES",
    collation: "MJB_FEATURE_COLLATION",
    idna: "MJB_FEATURE_IDNA",
    security: "MJB_FEATURE_SECURITY"
  }

  def project do
    [
      app: :mojibake,
      version: @version,
      elixir: "~> 1.15",
      start_permanent: Mix.env() == :prod,
      compilers: [:elixir_make] ++ Mix.compilers(),
      make_env: &make_env/0,
      make_clean: ["clean"],
      make_precompiler: {:nif, CCPrecompiler},
      make_precompiler_url: precompiled_url(),
      make_precompiler_filename: "mojibake_nif",
      make_precompiler_priv_paths: ["mojibake_nif.*"],
      make_precompiler_nif_versions: [versions: ["2.16"]],
      make_force_build: force_build?(),
      cc_precompiler: [
        compilers: @precompiled_targets,
        cleanup: "clean",
        allow_missing_compiler: true
      ],
      deps: deps(),
      aliases: aliases(),
      description: description(),
      package: package(),
      docs: docs(),
      name: "Mojibake",
      source_url: @source_url
    ]
  end

  def application do
    [extra_applications: []]
  end

  defp deps do
    [
      {:elixir_make, "~> 0.9", runtime: false},
      {:cc_precompiler, "~> 0.1.11", runtime: false},
      {:ex_doc, "~> 0.34", only: :dev, runtime: false}
    ]
  end

  # checksum.exs lists the precompiled NIFs and their checksums. `mix elixir_make.checksum --all`
  # downloads them from the release and writes it; without it the package always compiles.
  defp aliases do
    ["hex.publish": [&require_checksums/1, "hex.publish"]]
  end

  defp require_checksums(_args) do
    case File.read("checksum.exs") do
      {:ok, contents} when byte_size(contents) > 4 ->
        :ok

      _ ->
        Mix.raise(
          "checksum.exs is missing or empty: run `mix elixir_make.checksum --all` once the " <>
            "release has its precompiled NIFs"
        )
    end
  end

  defp description do
    "Unicode text processing: normalization, casing, segmentation, terminal width, character " <>
      "data, collation, IDNA, confusables, encodings, BCP 47 tags, and bidirectional text. " <>
      "Wraps the Mojibake C library."
  end

  defp package do
    [
      licenses: ["MIT"],
      links: %{"GitHub" => @source_url},
      files:
        ~w(lib c_src Makefile Makefile.win mix.exs README.md LICENSE .formatter.exs) ++
          Enum.filter(["checksum.exs"], &File.regular?/1)
    ]
  end

  defp docs do
    [
      main: "readme",
      extras: ["README.md"],
      groups_for_modules: [
        "Character data": [
          Mojibake.Char,
          Mojibake.Property,
          Mojibake.Script,
          Mojibake.Block,
          Mojibake.Plane,
          Mojibake.Emoji,
          Mojibake.Hangul
        ],
        "Sorting, domains, and security": [
          Mojibake.Collation,
          Mojibake.IDNA,
          Mojibake.Security,
          Mojibake.Identifier
        ],
        "Encodings and language tags": [
          Mojibake.Encoding,
          Mojibake.LanguageTag
        ],
        "Bidirectional text": [
          Mojibake.Bidi,
          Mojibake.Bidi.Paragraph
        ]
      ],
      source_url: @source_url,
      source_url_pattern: "#{@source_url}/blob/main/src/elixir/%{path}#L%{line}"
    ]
  end

  # MOJIBAKE_PRECOMPILED_URL replaces the release URL, for a mirror. It is a template like
  # @precompiled_url, with @{artefact_filename} for the file name.
  defp precompiled_url, do: System.get_env("MOJIBAKE_PRECOMPILED_URL") || @precompiled_url

  # Compile the NIF instead of downloading it when asked to with MOJIBAKE_BUILD, when the
  # precompiled NIFs would not match the sources or the features, or inside the repository, whose
  # sources are newer than the last release. `config :elixir_make, :force_build, mojibake: true`
  # forces it too.
  defp force_build? do
    System.get_env("MOJIBAKE_BUILD") in ["1", "true"] or
      System.get_env("MOJIBAKE_SRC_DIR") != nil or checkout?() or
      Enum.sort(enabled_features()) != Enum.sort(@features)
  end

  # Environment for the Makefile: where the C sources are, which to compile, and which optional
  # features to build.
  defp make_env do
    src = source_dir()
    sources = c_sources(src)
    defines = feature_defines()

    env = %{
      "MOJIBAKE_SRC_DIR" => src,
      "MOJIBAKE_SOURCES" => Enum.join(sources, " "),
      "MOJIBAKE_DEFINES" => defines
    }

    case :os.type() do
      {:win32, _} -> Map.merge(env, windows_env(src, sources, defines))
      _ -> env
    end
  end

  # nmake has no pattern rules, no functions, and no portable mkdir, so Makefile.win gets the
  # source paths and its directories ready-made, in Windows form. The flags file holds the
  # compiler flags and changes only when they do, so that a change rebuilds the NIF, like the
  # flags file of the Makefile.
  defp windows_env(src, sources, defines) do
    app_path = Mix.Project.app_path()
    obj_dir = Path.join(app_path, "obj")
    priv_dir = Path.join(app_path, "priv")
    flags_file = Path.join(obj_dir, "flags")
    flags = Enum.join([System.get_env("CFLAGS", ""), src, defines], "\n")

    File.mkdir_p!(obj_dir)
    File.mkdir_p!(priv_dir)

    if File.read(flags_file) != {:ok, flags} do
      File.write!(flags_file, flags)
    end

    %{
      "MOJIBAKE_SRC_DIR" => windows_path(src),
      "MOJIBAKE_SOURCE_PATHS" => Enum.map_join(sources, " ", &windows_path(Path.join(src, &1))),
      "MOJIBAKE_OBJ_DIR" => windows_path(obj_dir),
      "MOJIBAKE_PRIV_DIR" => windows_path(priv_dir),
      "MOJIBAKE_FLAGS_FILE" => windows_path(flags_file)
    }
  end

  defp windows_path(path), do: String.replace(path, "/", "\\")

  # Inside a Mojibake checkout, the repository's src/ directory.
  defp checkout? do
    checkout = Path.expand("..", __DIR__)

    Path.basename(__DIR__) == "elixir" and File.regular?(Path.join(checkout, "mojibake.h")) and
      File.regular?(Path.join(checkout, "CMakeLists.txt"))
  end

  # The C sources are read from, in order:
  #
  # 1. MOJIBAKE_SRC_DIR, when it is set
  # 2. the repository's src/ directory, when the package is built inside a Mojibake checkout
  # 3. c_src/mojibake/, the copy that `make vendor-elixir` puts in the package for Hex
  #
  # The checkout wins over the vendored copy, so in-tree builds never compile a stale copy.
  defp source_dir do
    checkout = Path.expand("..", __DIR__)
    vendored = Path.expand("c_src/mojibake", __DIR__)

    cond do
      dir = System.get_env("MOJIBAKE_SRC_DIR") ->
        Path.expand(dir)

      checkout?() ->
        checkout

      File.regular?(Path.join(vendored, "CMakeLists.txt")) ->
        vendored

      true ->
        Mix.raise(
          "cannot find the Mojibake C sources: set MOJIBAKE_SRC_DIR, build inside the " <>
            "repository, or run `make vendor-elixir` from the repository root to fill #{vendored}"
        )
    end
  end

  # The source list is the generated SOURCES block of CMakeLists.txt, so it cannot drift from the
  # CMake build.
  defp c_sources(src) do
    cmake = Path.join(src, "CMakeLists.txt")

    case Regex.run(~r/set\(SOURCES(.*?)\)/s, File.read!(cmake)) do
      [_, block] -> block |> String.split() |> Enum.filter(&String.ends_with?(&1, ".c"))
      nil -> Mix.raise("no SOURCES block in #{cmake}")
    end
  end

  # Optional C features. All are on by default. Turn some off in config/config.exs:
  #
  #     config :mojibake, features: [:collation, :idna]
  #
  # or with the MOJIBAKE_FEATURES environment variable, a comma-separated list or "none", which
  # wins over the configuration.
  defp enabled_features do
    case System.get_env("MOJIBAKE_FEATURES") do
      nil -> Application.get_env(:mojibake, :features, @features)
      "none" -> []
      value -> value |> String.split(",", trim: true) |> Enum.map(&parse_feature/1)
    end
  end

  defp feature_defines do
    enabled = enabled_features()

    Enum.each(enabled, fn feature ->
      if feature not in @features do
        Mix.raise(
          "unknown Mojibake feature #{inspect(feature)}; expected one of #{inspect(@features)}"
        )
      end
    end)

    @features
    |> Enum.map(fn feature ->
      "-D#{Map.fetch!(@feature_defines, feature)}=#{if feature in enabled, do: 1, else: 0}"
    end)
    |> Enum.join(" ")
  end

  defp parse_feature(name) do
    name = String.trim(name)
    Enum.find(@features, fn feature -> Atom.to_string(feature) == name end) || name
  end
end
