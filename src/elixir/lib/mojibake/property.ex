# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Property do
  @moduledoc """
  The Unicode properties that Mojibake stores per codepoint.

  A property is an atom made from its long name in `PropertyAliases.txt`, such as `:alphabetic`,
  `:white_space`, `:extended_pictographic`, or `:line_break`.

  Binary properties are true or false for each codepoint; query them with
  `Mojibake.Char.property?/2`. Enumerated properties have one value per codepoint, such as
  `:alphabetic` for the `:line_break` of "A"; query them with `Mojibake.Char.property/2`. Their
  values are atoms made from the long value names in `PropertyValueAliases.txt`.

  The enumerated properties are `:east_asian_width`, `:grapheme_cluster_break`,
  `:indic_conjunct_break`, `:joining_type`, `:line_break`, `:script`, `:sentence_break`, and
  `:word_break`. `General_Category`, `Bidi_Class`, `Canonical_Combining_Class`, and
  `Decomposition_Type` come from `Mojibake.Char.info/1` instead.
  """

  alias Mojibake.UnicodeData

  @typedoc "A property, such as `:alphabetic`."
  @type t :: atom()

  @properties UnicodeData.properties()
  @atoms @properties |> Enum.sort_by(fn {_, {id, _}} -> id end) |> Enum.map(&elem(&1, 0))

  @doc """
  Every property, binary and enumerated.

  ## Examples

      iex> :white_space in Mojibake.Property.all()
      true
  """
  @spec all() :: [t()]
  def all, do: @atoms

  @doc """
  Return `true` if `property` is a binary property, and `false` if it is an enumerated one or not
  a property.

  ## Examples

      iex> Mojibake.Property.binary?(:alphabetic)
      true

      iex> Mojibake.Property.binary?(:line_break)
      false
  """
  @spec binary?(t()) :: boolean()
  def binary?(property) when is_atom(property) do
    match?({:ok, {_, :binary}}, Map.fetch(@properties, property))
  end

  @doc """
  The values of the enumerated `property`, or `nil` if it is binary or not a property.

  ## Examples

      iex> Mojibake.Property.values(:east_asian_width)
      [:ambiguous, :fullwidth, :halfwidth, :neutral, :narrow, :wide]
  """
  @spec values(t()) :: [atom()] | nil
  def values(property) when is_atom(property) do
    case Map.fetch(@properties, property) do
      {:ok, {_, values}} when is_tuple(values) -> values |> Tuple.to_list() |> tl()
      _ -> nil
    end
  end

  @doc false
  # {mjb_property, :binary | values tuple}, or :error.
  def fetch(property), do: Map.fetch(@properties, property)
end
