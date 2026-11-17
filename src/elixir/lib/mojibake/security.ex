# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Security do
  @moduledoc """
  Confusable detection and mixed-script checks (UTS #39).

  Strings can look alike while being different: "pаypal" with a Cyrillic "а" is not "paypal".
  `confusable?/2` tells whether two strings look the same, and `skeleton/1` returns the form
  they are compared in, which you can store to look up lookalikes later.
  `resolved_scripts/1` and `mixed_script?/1` detect strings that mix scripts.

  These functions need the `:security` feature. Without it they return or raise `Mojibake.Error`
  with reason `:feature_not_enabled`.
  """

  import Mojibake.Internal

  alias Mojibake.{Error, Nif, Script}

  @doc """
  Return `true` if `first` and `second` are visually confusable: their skeletons are equal.

  Ill-formed UTF-8 compares as U+FFFD.

  ## Examples

      iex> Mojibake.Security.confusable?("paypal", "pаypal")
      true

      iex> Mojibake.Security.confusable?("rn", "m")
      true

      iex> Mojibake.Security.confusable?("paypal", "PayPal")
      false
  """
  @spec confusable?(String.t(), String.t()) :: boolean()
  def confusable?(first, second) when is_binary(first) and is_binary(second) do
    first |> Nif.confusable_match(second, :replace) |> value!()
  end

  @doc """
  Return the confusable skeleton of `string` (UTS #39 `bidiSkeleton` with left-to-right
  direction): its bidi visual order, decomposed, without default-ignorable characters, and with
  each character replaced by the prototype it is confusable with.

  A skeleton is for comparison only; do not show it.

  ## Examples

      iex> Mojibake.Security.skeleton("hеllo")
      {:ok, "hello"}

      iex> Mojibake.Security.skeleton(<<0xFF>>)
      {:error, %Mojibake.Error{reason: :malformed_input}}
  """
  @spec skeleton(String.t()) :: {:ok, String.t()} | {:error, Error.t()}
  def skeleton(string) when is_binary(string), do: string |> Nif.confusable_skeleton() |> result()

  @doc """
  Like `skeleton/1`, but raises `Mojibake.Error`.

  ## Examples

      iex> Mojibake.Security.skeleton!("ℌello")
      "Hello"
  """
  @spec skeleton!(String.t()) :: String.t()
  def skeleton!(string), do: bang(skeleton(string))

  @doc """
  Return the resolved script set of `string`: the scripts that all its characters share.

  Characters of the Common and Inherited scripts, such as digits and punctuation, go with any
  script. Han, Hiragana, Katakana, Hangul, and Bopomofo also count as the writing systems that
  use them, `:japanese`, `:korean`, and `:han_with_bopomofo`, so Japanese text that mixes kanji
  and kana resolves to `[:japanese]`, and Hangul text to `[:hangul, :korean]`.

  Returns `:all` for a string that has only Common and Inherited characters, or none, and `[]`
  for a mixed-script string. Raises `Mojibake.Error` if `string` is not well-formed UTF-8.

  ## Examples

      iex> Mojibake.Security.resolved_scripts("Ελλάδα")
      [:greek]

      iex> Mojibake.Security.resolved_scripts("日本語のテキスト")
      [:japanese]

      iex> Mojibake.Security.resolved_scripts("123!")
      :all

      iex> Mojibake.Security.resolved_scripts("pаypal")
      []
  """
  @spec resolved_scripts(String.t()) :: :all | [Script.t()]
  def resolved_scripts(string) when is_binary(string) do
    case string |> Nif.resolved_script_set() |> value!() do
      :all -> :all
      :empty -> []
      ids -> Enum.map(ids, &Script.from_id/1)
    end
  end

  @doc """
  Return `true` if `string` mixes scripts that no single writing system uses together, the
  UTS #39 definition of mixed-script. Raises `Mojibake.Error` if `string` is not well-formed UTF-8.

  ## Examples

      iex> Mojibake.Security.mixed_script?("pаypal")
      true

      iex> Mojibake.Security.mixed_script?("paypal2")
      false
  """
  @spec mixed_script?(String.t()) :: boolean()
  def mixed_script?(string), do: resolved_scripts(string) == []
end
