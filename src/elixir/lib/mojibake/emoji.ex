# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Emoji do
  @moduledoc """
  Emoji sequences (UTS #51).

  The emoji properties of a single codepoint come from `Mojibake.Char.emoji_properties/1`.
  """

  alias Mojibake.Nif

  @typedoc """
  The kind of an emoji sequence:

    * `:basic` - a single emoji, such as "😀"
    * `:keycap` - a keycap sequence, such as "1️⃣"
    * `:flag` - a pair of regional indicators, such as "🇮🇹"
    * `:tag` - a tag sequence, such as the flag of Scotland
    * `:modifier` - an emoji with a skin-tone modifier, such as "👍🏽"
    * `:zwj` - a sequence joined with ZERO WIDTH JOINER, such as "👨‍👩‍👧"
    * `:text_variation` - a character followed by VARIATION SELECTOR-15
    * `:emoji_variation` - a character followed by VARIATION SELECTOR-16
    * `nil` - a sequence that only `emoji-test.txt` lists, such as the unqualified keycap
      "1" U+20E3
  """
  @type sequence_type ::
          nil
          | :basic
          | :keycap
          | :flag
          | :tag
          | :modifier
          | :zwj
          | :text_variation
          | :emoji_variation

  @typedoc """
  The qualification status of an emoji sequence in `emoji-test.txt`: `:component`,
  `:fully_qualified`, `:minimally_qualified`, `:unqualified`, or `nil` for a sequence that
  `emoji-test.txt` does not list, such as a variation sequence.
  """
  @type qualification ::
          :component | :fully_qualified | :minimally_qualified | :unqualified | nil

  @typedoc "What `sequence/1` returns."
  @type sequence :: %{
          type: sequence_type(),
          qualification: qualification(),
          codepoint_count: pos_integer()
        }

  # Indexed by mjb_emoji_sequence_type and mjb_emoji_qualification.
  @types {nil, :basic, :keycap, :flag, :tag, :modifier, :zwj, :text_variation, :emoji_variation}
  @qualifications {nil, :component, :fully_qualified, :minimally_qualified, :unqualified}

  @doc """
  Describe `string` if the whole of it is one emoji sequence that Unicode lists, including the
  standardized emoji variation sequences. Return `nil` otherwise.

  ## Examples

      iex> Mojibake.Emoji.sequence("👍🏽")
      %{type: :modifier, qualification: :fully_qualified, codepoint_count: 2}

      iex> Mojibake.Emoji.sequence("🇮🇹")
      %{type: :flag, qualification: :fully_qualified, codepoint_count: 2}

      iex> Mojibake.Emoji.sequence("hi")
      nil
  """
  @spec sequence(String.t()) :: sequence() | nil
  def sequence(string) when is_binary(string) do
    case Nif.emoji_sequence(string) do
      {:ok, {type, qualification, count}} ->
        %{
          type: elem(@types, type),
          qualification: elem(@qualifications, qualification),
          codepoint_count: count
        }

      {:error, _} ->
        nil
    end
  end

  @doc """
  Return `true` if the whole of `string` is one emoji sequence that Unicode lists.

  ## Examples

      iex> Mojibake.Emoji.sequence?("👨‍👩‍👧")
      true

      iex> Mojibake.Emoji.sequence?("👍👍")
      false
  """
  @spec sequence?(String.t()) :: boolean()
  def sequence?(string) when is_binary(string), do: sequence(string) != nil

  @doc """
  Return `true` if `string` is a recommended-for-general-interchange (RGI) emoji: a basic,
  keycap, flag, tag, modifier, or ZWJ sequence, or a fully-qualified or component one.

  ## Examples

      iex> Mojibake.Emoji.rgi?("😀")
      true

      iex> Mojibake.Emoji.rgi?("#")
      false
  """
  @spec rgi?(String.t()) :: boolean()
  def rgi?(string) when is_binary(string) do
    case sequence(string) do
      %{type: type} when type in [:basic, :keycap, :flag, :tag, :modifier, :zwj] -> true
      %{qualification: qualification} -> qualification in [:component, :fully_qualified]
      nil -> false
    end
  end
end
