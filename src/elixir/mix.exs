# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.MixProject do
  use Mix.Project

  @version "0.7.0"
  @source_url "https://github.com/zaerl/mojibake"
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
      deps: deps(),
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
      {:ex_doc, "~> 0.34", only: :dev, runtime: false}
    ]
  end

  defp description do
    "Unicode text processing: normalization, locale-aware casing, caseless matching, grapheme, " <>
      "word, sentence, and line segmentation, and terminal width. Wraps the Mojibake C library."
  end

  defp package do
    [
      licenses: ["MIT"],
      links: %{"GitHub" => @source_url},
      files: ~w(lib c_src Makefile mix.exs README.md LICENSE .formatter.exs)
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
        ]
      ],
      source_url: @source_url,
      source_url_pattern: "#{@source_url}/blob/main/src/elixir/%{path}#L%{line}"
    ]
  end

  # Environment for the Makefile: where the C sources are, which to compile, and which optional
  # features to build.
  defp make_env do
    src = source_dir()

    %{
      "MOJIBAKE_SRC_DIR" => src,
      "MOJIBAKE_SOURCES" => Enum.join(c_sources(src), " "),
      "MOJIBAKE_DEFINES" => feature_defines()
    }
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

      Path.basename(__DIR__) == "elixir" and File.regular?(Path.join(checkout, "mojibake.h")) and
          File.regular?(Path.join(checkout, "CMakeLists.txt")) ->
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
  defp feature_defines do
    enabled =
      case System.get_env("MOJIBAKE_FEATURES") do
        nil -> Application.get_env(:mojibake, :features, @features)
        "none" -> []
        value -> value |> String.split(",", trim: true) |> Enum.map(&parse_feature/1)
      end

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
