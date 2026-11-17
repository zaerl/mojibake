# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Hangul do
  @moduledoc """
  Hangul syllables and jamo (Unicode Section 3.12).

  A precomposed Hangul syllable is a leading consonant (L, U+1100 to U+1112), a vowel (V,
  U+1161 to U+1175), and an optional trailing consonant (T, U+11A8 to U+11C2). Normalization
  composes and decomposes them automatically; these functions work on single syllables.
  """

  import Mojibake.Codepoint

  alias Mojibake.Nif

  @doc """
  Compose a syllable from a leading consonant, a vowel, and an optional trailing consonant.
  Return `nil` if a jamo is outside its range.

  ## Examples

      iex> Mojibake.Hangul.compose(0x1112, 0x1161, 0x11AB)
      0xD55C

      iex> Mojibake.Hangul.compose(0x1112, 0x1161)
      0xD558

      iex> Mojibake.Hangul.compose(?a, 0x1161)
      nil
  """
  @spec compose(non_neg_integer(), non_neg_integer(), non_neg_integer() | nil) ::
          non_neg_integer() | nil
  def compose(leading, vowel, trailing \\ nil)
      when is_codepoint(leading) and is_codepoint(vowel) and
             (is_nil(trailing) or is_codepoint(trailing)) do
    case Nif.hangul_compose(leading, vowel, trailing || 0) do
      {:ok, syllable} -> syllable
      {:error, _} -> nil
    end
  end

  @doc """
  Decompose a syllable into `{leading, vowel, trailing}`, with `trailing` `nil` when it has
  none. Return `nil` if `syllable` is not a Hangul syllable.

  ## Examples

      iex> Mojibake.Hangul.decompose(?한)
      {0x1112, 0x1161, 0x11AB}

      iex> Mojibake.Hangul.decompose(?하)
      {0x1112, 0x1161, nil}

      iex> Mojibake.Hangul.decompose(?a)
      nil
  """
  @spec decompose(non_neg_integer()) ::
          {non_neg_integer(), non_neg_integer(), non_neg_integer() | nil} | nil
  def decompose(syllable) when is_codepoint(syllable) do
    case Nif.hangul_decompose(syllable) do
      {:ok, {leading, vowel, 0}} -> {leading, vowel, nil}
      {:ok, jamo} -> jamo
      {:error, _} -> nil
    end
  end

  @doc """
  The character name of a Hangul syllable, derived as Unicode Section 3.12 describes. It does not
  need the character-name tables. Return `nil` if `syllable` is not a Hangul syllable.

  ## Examples

      iex> Mojibake.Hangul.syllable_name(?한)
      "HANGUL SYLLABLE HAN"
  """
  @spec syllable_name(non_neg_integer()) :: String.t() | nil
  def syllable_name(syllable) when is_codepoint(syllable) do
    case Nif.hangul_syllable_name(syllable) do
      {:ok, name} -> name
      {:error, _} -> nil
    end
  end

  @doc """
  Return `true` if `codepoint` is a precomposed Hangul syllable, U+AC00 to U+D7A3.

  ## Examples

      iex> Mojibake.Hangul.syllable?(?한)
      true
  """
  @spec syllable?(non_neg_integer()) :: boolean()
  def syllable?(codepoint) when is_codepoint(codepoint),
    do: Nif.codepoint_is(codepoint, :hangul_syllable)

  @doc """
  Return `true` if `codepoint` is a conjoining jamo: a leading consonant, a vowel, or a trailing
  consonant.

  ## Examples

      iex> Mojibake.Hangul.jamo?(0x1100)
      true

      iex> Mojibake.Hangul.jamo?(?한)
      false
  """
  @spec jamo?(non_neg_integer()) :: boolean()
  def jamo?(codepoint) when is_codepoint(codepoint),
    do: Nif.codepoint_is(codepoint, :hangul_jamo)

  @doc """
  Return `true` if `codepoint` is a leading consonant jamo (L) that composes into a syllable,
  U+1100 to U+1112.
  """
  @spec leading_jamo?(non_neg_integer()) :: boolean()
  def leading_jamo?(codepoint) when is_codepoint(codepoint),
    do: Nif.codepoint_is(codepoint, :hangul_leading_jamo)

  @doc """
  Return `true` if `codepoint` is a vowel jamo (V) that composes into a syllable, U+1161 to
  U+1175.
  """
  @spec vowel_jamo?(non_neg_integer()) :: boolean()
  def vowel_jamo?(codepoint) when is_codepoint(codepoint),
    do: Nif.codepoint_is(codepoint, :hangul_vowel_jamo)

  @doc """
  Return `true` if `codepoint` is a trailing consonant jamo (T) that composes into a syllable,
  U+11A8 to U+11C2.
  """
  @spec trailing_jamo?(non_neg_integer()) :: boolean()
  def trailing_jamo?(codepoint) when is_codepoint(codepoint),
    do: Nif.codepoint_is(codepoint, :hangul_trailing_jamo)
end
