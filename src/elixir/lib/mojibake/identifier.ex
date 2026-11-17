# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Identifier do
  @moduledoc """
  Unicode identifiers and pattern syntax (UAX #31).

  An identifier is a start character followed by any number of continue characters, as in most
  programming languages, but with every script: "café", "δ", and "変数" are identifiers.

  The `:default` profile uses the ID_Start and ID_Continue properties. The `:nfkc` profile uses
  XID_Start and XID_Continue, which stay identifiers after NFKC normalization; use it when you
  normalize identifiers, as Elixir, Rust, and Python do.
  """

  import Mojibake.Codepoint

  alias Mojibake.Nif

  @typedoc "An identifier profile."
  @type profile :: :default | :nfkc

  @doc """
  Return `true` if `string` is an identifier in `profile`. The empty string and ill-formed UTF-8
  are not identifiers.

  ## Examples

      iex> Mojibake.Identifier.valid?("café_2")
      true

      iex> Mojibake.Identifier.valid?("2fast")
      false

      iex> Mojibake.Identifier.valid?("a-b")
      false
  """
  @spec valid?(String.t(), profile()) :: boolean()
  def valid?(string, profile \\ :default)
      when is_binary(string) and profile in [:default, :nfkc] do
    Nif.is_identifier(string, profile)
  end

  @doc """
  Return `true` if `codepoint` can start an identifier in `profile`: ID_Start or XID_Start.

  ## Examples

      iex> Mojibake.Identifier.start?(?α)
      true

      iex> Mojibake.Identifier.start?(?_)
      false
  """
  @spec start?(non_neg_integer(), profile()) :: boolean()
  def start?(codepoint, profile \\ :default)

  def start?(codepoint, :default) when is_codepoint(codepoint),
    do: Nif.codepoint_is(codepoint, :id_start)

  def start?(codepoint, :nfkc) when is_codepoint(codepoint),
    do: Nif.codepoint_is(codepoint, :xid_start)

  @doc """
  Return `true` if `codepoint` can continue an identifier in `profile`: ID_Continue or
  XID_Continue.

  ## Examples

      iex> Mojibake.Identifier.continue?(?_)
      true

      iex> Mojibake.Identifier.continue?(?-)
      false
  """
  @spec continue?(non_neg_integer(), profile()) :: boolean()
  def continue?(codepoint, profile \\ :default)

  def continue?(codepoint, :default) when is_codepoint(codepoint),
    do: Nif.codepoint_is(codepoint, :id_continue)

  def continue?(codepoint, :nfkc) when is_codepoint(codepoint),
    do: Nif.codepoint_is(codepoint, :xid_continue)

  @doc """
  Return `true` if `codepoint` is reserved for syntax in patterns and programming languages
  (Pattern_Syntax), such as `+` or `→`.

  ## Examples

      iex> Mojibake.Identifier.pattern_syntax?(?+)
      true
  """
  @spec pattern_syntax?(non_neg_integer()) :: boolean()
  def pattern_syntax?(codepoint) when is_codepoint(codepoint),
    do: Nif.codepoint_is(codepoint, :pattern_syntax)

  @doc """
  Return `true` if `codepoint` is white space in patterns (Pattern_White_Space).

  ## Examples

      iex> Mojibake.Identifier.pattern_white_space?(?\\t)
      true
  """
  @spec pattern_white_space?(non_neg_integer()) :: boolean()
  def pattern_white_space?(codepoint) when is_codepoint(codepoint),
    do: Nif.codepoint_is(codepoint, :pattern_white_space)
end
