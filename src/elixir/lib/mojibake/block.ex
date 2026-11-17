# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Block do
  @moduledoc """
  Unicode blocks (`Blocks.txt`).

  A block is an atom made from its name: `:basic_latin` for "Basic Latin", `:latin_1_supplement`
  for "Latin-1 Supplement". `Mojibake.Char.block/1` returns the block of a codepoint.
  """

  alias Mojibake.UnicodeData

  @typedoc "A block, such as `:basic_latin`."
  @type t :: atom()

  @blocks UnicodeData.blocks() |> Tuple.to_list()
  @atoms Enum.map(@blocks, &elem(&1, 0))
  @info Map.new(@blocks, fn {block, name, first, last} -> {block, {name, first, last}} end)

  @doc """
  Every block, in codepoint order.

  ## Examples

      iex> Mojibake.Block.all() |> hd()
      :basic_latin
  """
  @spec all() :: [t()]
  def all, do: @atoms

  @doc """
  The name of `block` in `Blocks.txt`, or `nil` if `block` is not a block.

  ## Examples

      iex> Mojibake.Block.name(:latin_1_supplement)
      "Latin-1 Supplement"
  """
  @spec name(t()) :: String.t() | nil
  def name(block) when is_atom(block) do
    case Map.fetch(@info, block) do
      {:ok, {name, _, _}} -> name
      :error -> nil
    end
  end

  @doc """
  The codepoints of `block`, or `nil` if `block` is not a block.

  ## Examples

      iex> Mojibake.Block.range(:basic_latin)
      0..127

      iex> 0x3B1 in Mojibake.Block.range(:greek_and_coptic)
      true
  """
  @spec range(t()) :: Range.t() | nil
  def range(block) when is_atom(block) do
    case Map.fetch(@info, block) do
      {:ok, {_, first, last}} -> first..last//1
      :error -> nil
    end
  end

  @doc false
  # The block for a mjb_block value.
  def from_id(id) when is_integer(id) and id >= 0 and id < length(@blocks) do
    UnicodeData.blocks() |> elem(id) |> elem(0)
  end

  def from_id(_id), do: nil
end
