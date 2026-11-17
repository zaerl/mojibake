# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

# Usage: elixir src/elixir/scripts/check_precompiled.exs ARCHIVE_DIR
#
# Check that the Hex package installs the precompiled NIF for this host from the archives that
# `mix elixir_make.precompile` wrote to ARCHIVE_DIR, without compiling anything, and that the NIF
# loads. The archives go in elixir_make's cache, so nothing is downloaded. Needs the package's
# dependencies (`mix deps.get` in src/elixir).

defmodule CheckPrecompiled do
  @root Path.expand("../../..", __DIR__)
  @package_dir Path.join(@root, "src/elixir")

  def main([archive_dir]) do
    archives = archive_dir |> Path.expand() |> Path.join("*.tar.gz") |> Path.wildcard()

    if archives == [] do
      fail("no precompiled archives in #{archive_dir}")
    end

    work =
      Path.join(System.tmp_dir!(), "mojibake-precompiled-#{System.unique_integer([:positive])}")

    cache = Path.join(work, "cache")
    package = Path.join(work, "package")

    try do
      File.mkdir_p!(cache)
      Enum.each(archives, &File.cp!(&1, Path.join(cache, Path.basename(&1))))

      build_package(package)
      File.write!(Path.join(package, "checksum.exs"), checksums(archives))
      IO.puts(File.read!(Path.join(package, "checksum.exs")))

      mix(["deps.get", "--only", "prod"], package, [{"MIX_ENV", "prod"}])

      # CC=false makes a source build fail on Unix; the missing object directory proves it on
      # Windows too.
      # A file, not -e: cmd.exe would strip the quotes from the code.
      File.write!(Path.join(package, "smoke_test.exs"), smoke_test())

      mix(["run", "--no-start", "smoke_test.exs"], package, [
        {"MIX_ENV", "prod"},
        {"ELIXIR_MAKE_CACHE_DIR", cache},
        {"CC", "false"},
        {"MOJIBAKE_BUILD", nil},
        {"MOJIBAKE_FEATURES", nil},
        {"MOJIBAKE_SRC_DIR", nil}
      ])

      app_path = Path.join(package, "_build/prod/lib/mojibake")

      if File.exists?(Path.join(app_path, "obj")) do
        fail("the NIF was compiled instead of installed from the precompiled archive")
      end

      IO.puts("priv: #{inspect(File.ls!(Path.join(app_path, "priv")))}")
      IO.puts("The precompiled NIF installs and loads.")
    after
      File.rm_rf(work)
    end
  end

  def main(_args) do
    IO.puts(:stderr, "usage: elixir src/elixir/scripts/check_precompiled.exs ARCHIVE_DIR")
    System.halt(2)
  end

  # Vendor the C sources and unpack the package outside the repository, as Hex users get it.
  defp build_package(package) do
    vendored = Path.join(@package_dir, "c_src/mojibake")
    File.rm_rf!(vendored)
    File.mkdir_p!(vendored)

    for file <-
          Path.wildcard(Path.join(@root, "src/*.{c,h}")) ++ [Path.join(@root, "src/CMakeLists.txt")] do
      File.cp!(file, Path.join(vendored, Path.basename(file)))
    end

    File.cp!(Path.join(@root, "LICENSE"), Path.join(@package_dir, "LICENSE"))
    mix(["hex.build", "--unpack", "-o", package], @package_dir, [])
  end

  # checksum.exs in the format of `mix elixir_make.checksum`.
  defp checksums(archives) do
    lines =
      for path <- Enum.sort(archives) do
        checksum = :sha256 |> :crypto.hash(File.read!(path)) |> Base.encode16(case: :lower)
        ~s(  "#{Path.basename(path)}" => "sha256:#{checksum}",\n)
      end

    IO.iodata_to_binary(["%{\n", lines, "}\n"])
  end

  defp smoke_test do
    ~S"""
    IO.puts("Mojibake #{Mojibake.version()}, features #{inspect(Mojibake.features())}")
    "abc גבא" = Mojibake.Bidi.visual("abc אבג")
    {:ok, "é"} = Mojibake.normalize("é")
    """
  end

  # mix is a batch file on Windows, which only cmd.exe runs.
  defp mix(args, dir, env) do
    IO.puts("$ mix #{Enum.join(args, " ")}")

    {command, command_args} =
      case :os.type() do
        {:win32, _} -> {"cmd", ["/c", "mix" | args]}
        _ -> {"mix", args}
      end

    case System.cmd(command, command_args,
           cd: dir,
           env: env,
           into: IO.stream(),
           stderr_to_stdout: true
         ) do
      {_, 0} -> :ok
      {_, status} -> fail("mix #{Enum.join(args, " ")} exited with status #{status}")
    end
  end

  defp fail(message) do
    IO.puts(:stderr, "error: " <> message)
    System.halt(1)
  end
end

CheckPrecompiled.main(System.argv())
