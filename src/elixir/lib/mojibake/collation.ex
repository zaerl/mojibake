# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Collation do
  @moduledoc """
  Language-independent sorting with the Unicode Collation Algorithm (UTS #10) and the Default
  Unicode Collation Element Table (DUCET).

  Codepoint order, which `Enum.sort/1` uses for strings, puts "Z" before "a" and "é" after "z".
  Collation sorts text the way people expect: letters with accents next to their base letters,
  and case as a later tie-breaker.

      iex> Enum.sort(["zebra", "éclair", "Apple", "apple"])
      ["Apple", "apple", "zebra", "éclair"]

      iex> Mojibake.Collation.sort(["zebra", "éclair", "Apple", "apple"])
      ["apple", "Apple", "éclair", "zebra"]

  `compare/2` returns `:lt`, `:eq`, or `:gt`, so the module itself works as an `Enum.sort/2`
  sorter:

      iex> Enum.sort(["b", "A", "a"], Mojibake.Collation)
      ["a", "A", "b"]

  DUCET is not tailored to any language: Swedish "ä" sorts like "a", not after "z". Collation is
  not caseless matching either; use `Mojibake.caseless_equal?/3` to compare strings while
  ignoring case.

  ## Options

    * `:strength` - how many levels to compare:
      * `:primary` - base letters only: "a", "á", and "A" are equal
      * `:secondary` - also accents: "a" and "á" differ, "a" and "A" do not
      * `:tertiary` (the default) - also case and other variants
      * `:quaternary` - also the punctuation that `variable: :shifted` sets aside
    * `:variable` - how punctuation and some symbols are weighed:
      * `:non_ignorable` (the default) - like letters
      * `:shifted` - ignored up to tertiary strength, so "ab" and "a-b" are equal; compared
        last at quaternary strength
    * `:malformed` - what to do with input that is not well-formed UTF-8: `:replace` (the
      default) compares each ill-formed subsequence as U+FFFD, `:skip` ignores it, and `:stop`
      raises `Mojibake.Error`

  These functions need the `:collation` feature, and raise `Mojibake.Error` with reason
  `:feature_not_enabled` without it.
  """

  import Mojibake.Internal

  alias Mojibake.Nif

  @strengths [:primary, :secondary, :tertiary, :quaternary]
  @variables [:non_ignorable, :shifted]

  @typedoc "A collation option. See the module documentation."
  @type option ::
          {:strength, :primary | :secondary | :tertiary | :quaternary}
          | {:variable, :non_ignorable | :shifted}
          | {:malformed, :stop | :replace | :skip}

  @doc """
  Compare two strings: `:lt` if `first` sorts before `second`, `:gt` if after, and `:eq` if they
  are equal at the selected strength.

  ## Examples

      iex> Mojibake.Collation.compare("apple", "Banana")
      :lt

      iex> Mojibake.Collation.compare("resume", "résumé", strength: :primary)
      :eq

      iex> Mojibake.Collation.compare("ab", "a-b", variable: :shifted)
      :eq
  """
  @spec compare(String.t(), String.t(), [option()]) :: :lt | :eq | :gt
  def compare(first, second, opts \\ [])
      when is_binary(first) and is_binary(second) and is_list(opts) do
    {variable, strength, policy} = options(opts)

    case first |> Nif.collation_compare(second, variable, strength, policy) |> value!() do
      -1 -> :lt
      0 -> :eq
      1 -> :gt
    end
  end

  @doc """
  Return `true` if the two strings are equal at the selected strength.

  ## Examples

      iex> Mojibake.Collation.equal?("Café", "cafe", strength: :primary)
      true

      iex> Mojibake.Collation.equal?("Café", "cafe")
      false
  """
  @spec equal?(String.t(), String.t(), [option()]) :: boolean()
  def equal?(first, second, opts \\ []), do: compare(first, second, opts) == :eq

  @doc """
  Return the sort key of `string`: a binary whose byte order is the collation order of the
  strings. Comparing keys with `<`, `>`, and `==` gives the same result as `compare/3`.

  Compute keys once when you sort or index many strings. Keys depend on the options, the
  Mojibake version, and the Unicode version, so do not store them across upgrades. Strings with
  no weights at the selected strength, such as the empty string, have an empty key.

  ## Examples

      iex> Mojibake.Collation.sort_key("a") < Mojibake.Collation.sort_key("b")
      true

      iex> Mojibake.Collation.sort_key("")
      ""
  """
  @spec sort_key(String.t(), [option()]) :: binary()
  def sort_key(string, opts \\ []) when is_binary(string) and is_list(opts) do
    {variable, strength, policy} = options(opts)

    string |> Nif.collation_key(variable, strength, policy) |> value!()
  end

  @doc """
  Sort `strings` in collation order, computing each sort key once. The sort is stable: strings
  that are equal at the selected strength keep their order.

  ## Examples

      iex> Mojibake.Collation.sort(["côte", "coté", "cote", "côté"])
      ["cote", "coté", "côte", "côté"]

      iex> Mojibake.Collation.sort(["b", "A", "a"], strength: :secondary)
      ["A", "a", "b"]
  """
  @spec sort([String.t()], [option()]) :: [String.t()]
  def sort(strings, opts \\ []) when is_list(strings) and is_list(opts) do
    Enum.sort_by(strings, &sort_key(&1, opts))
  end

  defp options(opts) do
    {option(opts, :variable, :non_ignorable, @variables),
     option(opts, :strength, :tertiary, @strengths), malformed_option(opts, :replace)}
  end
end
