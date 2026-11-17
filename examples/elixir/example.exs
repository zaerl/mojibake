# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

# This is a simple Elixir example of how to use the Mojibake library through the Elixir package in
# src/elixir. Run `make example-elixir` from the examples directory, or
# `elixir elixir/example.exs`. The first run compiles the NIF, which takes a while.
#
# An application depends on the package from Hex instead:
#
#     def deps do
#       [{:mojibake, "~> 0.7"}]
#     end
Mix.install([{:mojibake, path: Path.expand("../../src/elixir", __DIR__)}])

defmodule Example do
  # Print the bytes of a string: printable ASCII as is, everything else as <XX>.
  def print_bytes(string) do
    string
    |> :binary.bin_to_list()
    |> Enum.map_join(fn
      byte when byte in 0x21..0x7E -> <<byte>>
      byte -> "<" <> Base.encode16(<<byte>>) <> ">"
    end)
    |> IO.puts()
  end
end

input = "Café"

# Normalize example: in NFC e + ◌́ -> é (U+00E9)
normalized = Mojibake.normalize!(input, :nfc)

# Cafe + ◌́ (U+0301, COMBINING ACUTE ACCENT) -> Café
Example.print_bytes(input)

# Caf + é (U+00E9, LATIN SMALL LETTER E WITH ACUTE) -> Café
Example.print_bytes(normalized)

mojibake = "文字化け"

# Length example: byte_size counts bytes, String.length counts graphemes
IO.puts(
  "\"#{mojibake}\" encoded in UTF-8 is #{byte_size(mojibake)} bytes long, " <>
    "#{Mojibake.grapheme_count(mojibake)} graphemes long, and " <>
    "#{Mojibake.terminal_width(mojibake)} terminal columns wide"
)

case_input = "Straße"

# NFKC casefold example: in NFKC casefold, ß -> ss
IO.puts("#{case_input} -> #{Mojibake.normalize!(case_input, :nfkc_casefold)}")

# Locale-aware casing: Turkish has a dotted capital I
IO.puts("istanbul -> #{Mojibake.upcase!("istanbul", locale: "tr")}")

# Character data
IO.puts("U+1F600 is #{Mojibake.Char.name(0x1F600) || "unnamed"}")

# Bidirectional text: the visual order of mixed left-to-right and right-to-left text
IO.puts("abc אבג in visual order is #{Mojibake.Bidi.visual("abc אבג")}")

# Collation needs the :collation feature, which is on by default
if :collation in Mojibake.features() do
  sorted = Mojibake.Collation.sort(["zebra", "éclair", "Apple", "apple"])
  IO.puts("Sorted: #{Enum.join(sorted, ", ")}")
end
