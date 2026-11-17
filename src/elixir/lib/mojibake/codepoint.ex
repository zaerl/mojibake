# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Codepoint do
  @moduledoc false

  @doc false
  # A Unicode codepoint, U+0000 to U+10FFFF. Surrogates are codepoints too.
  defguard is_codepoint(value) when is_integer(value) and value >= 0 and value <= 0x10FFFF
end
