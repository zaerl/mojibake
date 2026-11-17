# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Char do
  @moduledoc """
  Character data: the Unicode Character Database properties of single codepoints.

  Every function takes a codepoint, an integer from 0 to 0x10FFFF, such as `?A` or `0x1F600`.
  Use `String.to_charlist/1` or `String.codepoints/1` to get the codepoints of a string.

  Values that the UCD names, such as categories, scripts, blocks, and property values, are atoms
  made from their long names: `:uppercase_letter`, `:latin`, `:basic_latin`, `:alphabetic`.

  ## Examples

      iex> Mojibake.Char.category(?A)
      :uppercase_letter

      iex> Mojibake.Char.script(?Ω)
      :greek

      iex> Mojibake.Char.property?(?7, :ascii_hex_digit)
      true
  """

  import Mojibake.Codepoint

  alias Mojibake.{Block, Nif, Property, Script, UnicodeData}

  @typedoc "A codepoint, 0 to 0x10FFFF."
  @type codepoint :: non_neg_integer()

  @typedoc """
  A General_Category value, such as `:uppercase_letter`, or `:unassigned`. See
  `category_abbreviation/1` for the two-letter form.
  """
  @type category :: atom()

  @typedoc """
  A Bidi_Class value: `:left_to_right`, `:right_to_left`, `:arabic_letter`, `:european_number`,
  `:european_separator`, `:european_terminator`, `:arabic_number`, `:common_separator`,
  `:nonspacing_mark`, `:boundary_neutral`, `:paragraph_separator`, `:segment_separator`,
  `:white_space`, `:other_neutral`, `:left_to_right_embedding`, `:left_to_right_override`,
  `:right_to_left_embedding`, `:right_to_left_override`, `:pop_directional_format`,
  `:left_to_right_isolate`, `:right_to_left_isolate`, `:first_strong_isolate`, or
  `:pop_directional_isolate`.
  """
  @type bidi_class :: atom()

  @typedoc """
  A Decomposition_Type value other than None: `:canonical`, or one of the compatibility types
  `:circle`, `:compat`, `:final`, `:font`, `:fraction`, `:initial`, `:isolated`, `:medial`,
  `:narrow`, `:no_break`, `:small`, `:square`, `:sub`, `:super`, `:vertical`, and `:wide`.
  """
  @type decomposition_type :: atom()

  @typedoc """
  A Numeric_Value: an integer, or `{numerator, denominator}` for a fraction such as ½.
  """
  @type numeric_value :: integer() | {integer(), pos_integer()}

  @typedoc "What `info/1` returns."
  @type t :: %__MODULE__{
          codepoint: codepoint(),
          name: String.t(),
          category: category(),
          combining_class: 0..254,
          bidi_class: bidi_class(),
          bidi_mirrored: boolean(),
          decomposition_type: decomposition_type() | nil,
          numeric_type: :decimal | :digit | :numeric | nil,
          numeric_value: numeric_value() | nil,
          uppercase: codepoint(),
          lowercase: codepoint(),
          titlecase: codepoint()
        }

  @enforce_keys [:codepoint]
  defstruct [
    :codepoint,
    :name,
    :category,
    :bidi_class,
    :decomposition_type,
    :numeric_type,
    :numeric_value,
    :uppercase,
    :lowercase,
    :titlecase,
    combining_class: 0,
    bidi_mirrored: false
  ]

  @categories UnicodeData.categories()

  # Indexed by mjb_bidi_class, which is hand-written in src/mojibake.h rather than generated.
  # c_src/mojibake_nif.c checks its size.
  @bidi_classes {
    nil,
    :left_to_right,
    :right_to_left,
    :arabic_letter,
    :european_number,
    :european_separator,
    :european_terminator,
    :arabic_number,
    :common_separator,
    :nonspacing_mark,
    :boundary_neutral,
    :paragraph_separator,
    :segment_separator,
    :white_space,
    :other_neutral,
    :left_to_right_embedding,
    :left_to_right_override,
    :right_to_left_embedding,
    :right_to_left_override,
    :pop_directional_format,
    :left_to_right_isolate,
    :right_to_left_isolate,
    :first_strong_isolate,
    :pop_directional_isolate
  }

  @doc """
  Return everything `UnicodeData.txt` says about `codepoint`, or `nil` if it is unassigned.

  Like the C library, `info/1` also returns `nil` for surrogates, private-use codepoints, and
  noncharacters; `category/1` still reports `:surrogate` and `:private_use` for them.

  Codepoints whose data `UnicodeData.txt` gives as a range, such as CJK ideographs and Hangul
  syllables, have their derived names.

  `uppercase`, `lowercase`, and `titlecase` are the simple (single-codepoint) case mappings, and
  are the codepoint itself when it has none.

  ## Examples

      iex> Mojibake.Char.info(?½)
      %Mojibake.Char{
        codepoint: 0xBD,
        name: "VULGAR FRACTION ONE HALF",
        category: :other_number,
        combining_class: 0,
        bidi_class: :other_neutral,
        bidi_mirrored: false,
        decomposition_type: :fraction,
        numeric_type: :numeric,
        numeric_value: {1, 2},
        uppercase: 0xBD,
        lowercase: 0xBD,
        titlecase: 0xBD
      }

      iex> Mojibake.Char.info(0x0378)
      nil
  """
  @spec info(codepoint()) :: t() | nil
  def info(codepoint) when is_codepoint(codepoint) do
    case Nif.codepoint_info(codepoint) do
      {:ok, info} -> to_struct(codepoint, info)
      {:error, _} -> nil
    end
  end

  @doc """
  The character name of `codepoint`, or `nil` if it is unassigned. Characters without a name of
  their own, such as controls, have the empty string.

  Needs the `:names` feature; raises `Mojibake.Error` with reason `:feature_not_enabled` when the
  package is built without it. Hangul syllable names are available from
  `Mojibake.Hangul.syllable_name/1` either way.

  ## Examples

      iex> Mojibake.Char.name(?A)
      "LATIN CAPITAL LETTER A"

      iex> Mojibake.Char.name(0x4E00)
      "CJK UNIFIED IDEOGRAPH-4E00"
  """
  @spec name(codepoint()) :: String.t() | nil
  def name(codepoint) when is_codepoint(codepoint) do
    unless :names in Mojibake.features() do
      raise Mojibake.Error, reason: :feature_not_enabled
    end

    case Nif.codepoint_info(codepoint) do
      {:ok, info} -> elem(info, 0)
      {:error, _} -> nil
    end
  end

  @doc """
  The General_Category of `codepoint`, `:unassigned` if it has none. Noncharacters, such as
  U+FFFF, are `:unassigned`.

  ## Examples

      iex> Mojibake.Char.category(?a)
      :lowercase_letter

      iex> Mojibake.Char.category(0x0378)
      :unassigned
  """
  @spec category(codepoint()) :: category()
  def category(codepoint) when is_codepoint(codepoint) do
    case Nif.codepoint_info(codepoint) do
      {:ok, info} -> @categories |> elem(elem(info, 1)) |> elem(0)
      {:error, _} -> fallback_category(codepoint)
    end
  end

  @doc """
  The two-letter General_Category abbreviation of `codepoint`, such as `"Lu"`, or `"Cn"` if it is
  unassigned.

  ## Examples

      iex> Mojibake.Char.category_abbreviation(?A)
      "Lu"
  """
  @spec category_abbreviation(codepoint()) :: String.t()
  def category_abbreviation(codepoint) when is_codepoint(codepoint) do
    case Nif.codepoint_info(codepoint) do
      {:ok, info} -> @categories |> elem(elem(info, 1)) |> elem(1)
      {:error, _} -> codepoint |> fallback_category() |> abbreviation()
    end
  end

  @doc """
  The Canonical_Combining_Class of `codepoint`, 0 for starters and unassigned codepoints.

  ## Examples

      iex> Mojibake.Char.combining_class(0x0301)
      230
  """
  @spec combining_class(codepoint()) :: 0..254
  def combining_class(codepoint) when is_codepoint(codepoint) do
    case Nif.codepoint_info(codepoint) do
      {:ok, info} -> elem(info, 2)
      {:error, _} -> 0
    end
  end

  @doc """
  The Bidi_Class of `codepoint` in `UnicodeData.txt`, or `nil` if it is unassigned.

  ## Examples

      iex> Mojibake.Char.bidi_class(?A)
      :left_to_right

      iex> Mojibake.Char.bidi_class(0x05D0)
      :right_to_left
  """
  @spec bidi_class(codepoint()) :: bidi_class() | nil
  def bidi_class(codepoint) when is_codepoint(codepoint) do
    case Nif.codepoint_info(codepoint) do
      {:ok, info} -> elem(@bidi_classes, elem(info, 3))
      {:error, _} -> nil
    end
  end

  @doc """
  Return `true` if `codepoint` is mirrored in bidirectional text (Bidi_Mirrored), such as `(`.

  ## Examples

      iex> Mojibake.Char.bidi_mirrored?(?()
      true
  """
  @spec bidi_mirrored?(codepoint()) :: boolean()
  def bidi_mirrored?(codepoint) when is_codepoint(codepoint) do
    match?({:ok, info} when elem(info, 8), Nif.codepoint_info(codepoint))
  end

  @doc """
  The Decomposition_Type of `codepoint`, or `nil` if it has no decomposition.

  ## Examples

      iex> Mojibake.Char.decomposition_type(?é)
      :canonical

      iex> Mojibake.Char.decomposition_type(?ﬁ)
      :compat

      iex> Mojibake.Char.decomposition_type(?a)
      nil
  """
  @spec decomposition_type(codepoint()) :: decomposition_type() | nil
  def decomposition_type(codepoint) when is_codepoint(codepoint) do
    case Nif.codepoint_info(codepoint) do
      {:ok, info} -> elem(UnicodeData.decomposition_types(), elem(info, 4))
      {:error, _} -> nil
    end
  end

  @doc """
  The Numeric_Type of `codepoint`: `:decimal` for decimal digits, `:digit` for other digits such
  as superscripts, `:numeric` for other numbers such as fractions and Roman numerals, or `nil`.

  ## Examples

      iex> Mojibake.Char.numeric_type(?7)
      :decimal

      iex> Mojibake.Char.numeric_type(?²)
      :digit

      iex> Mojibake.Char.numeric_type(?Ⅻ)
      :numeric
  """
  @spec numeric_type(codepoint()) :: :decimal | :digit | :numeric | nil
  def numeric_type(codepoint) when is_codepoint(codepoint) do
    case Nif.numeric_value(codepoint) do
      {:ok, numeric} -> numeric |> numeric() |> elem(0)
      {:error, _} -> nil
    end
  end

  @doc """
  The Numeric_Value of `codepoint`: an integer, `{numerator, denominator}` for a fraction, or
  `nil` if it is not a number.

  ## Examples

      iex> Mojibake.Char.numeric_value(?7)
      7

      iex> Mojibake.Char.numeric_value(?¾)
      {3, 4}

      iex> Mojibake.Char.numeric_value(?Ⅻ)
      12

      iex> Mojibake.Char.numeric_value(?a)
      nil
  """
  @spec numeric_value(codepoint()) :: numeric_value() | nil
  def numeric_value(codepoint) when is_codepoint(codepoint) do
    case Nif.numeric_value(codepoint) do
      {:ok, numeric} -> numeric |> numeric() |> elem(1)
      {:error, _} -> nil
    end
  end

  @doc """
  The simple uppercase mapping of `codepoint`: the single codepoint `UnicodeData.txt` gives, or
  `codepoint` itself. `Mojibake.upcase/2` applies the full mappings, such as "ß" to "SS".

  ## Examples

      iex> Mojibake.Char.simple_upcase(?a)
      ?A

      iex> Mojibake.Char.simple_upcase(?ß)
      ?ß
  """
  @spec simple_upcase(codepoint()) :: codepoint()
  def simple_upcase(codepoint) when is_codepoint(codepoint), do: mapping(codepoint, 9)

  @doc """
  The simple lowercase mapping of `codepoint`, or `codepoint` itself.

  ## Examples

      iex> Mojibake.Char.simple_downcase(?Ω)
      ?ω
  """
  @spec simple_downcase(codepoint()) :: codepoint()
  def simple_downcase(codepoint) when is_codepoint(codepoint), do: mapping(codepoint, 10)

  @doc """
  The simple titlecase mapping of `codepoint`, or `codepoint` itself.

  ## Examples

      iex> Mojibake.Char.simple_titlecase(?ǆ)
      ?ǅ
  """
  @spec simple_titlecase(codepoint()) :: codepoint()
  def simple_titlecase(codepoint) when is_codepoint(codepoint), do: mapping(codepoint, 11)

  @doc """
  The block of `codepoint`, or `nil` if it is in no block. See `Mojibake.Block`.

  ## Examples

      iex> Mojibake.Char.block(?A)
      :basic_latin

      iex> Mojibake.Char.block(0x1F600)
      :emoticons
  """
  @spec block(codepoint()) :: Block.t() | nil
  def block(codepoint) when is_codepoint(codepoint) do
    case Nif.block(codepoint) do
      {:ok, id} -> Block.from_id(id)
      {:error, _} -> nil
    end
  end

  @doc """
  The Script of `codepoint`, `:unknown` if it has none. See `Mojibake.Script`.

  ## Examples

      iex> Mojibake.Char.script(?A)
      :latin

      iex> Mojibake.Char.script(?1)
      :common
  """
  @spec script(codepoint()) :: Script.t()
  def script(codepoint) when is_codepoint(codepoint) do
    codepoint |> Nif.script() |> Script.from_id() || :unknown
  end

  @doc """
  The Script_Extensions of `codepoint`: the scripts it is used with. For most codepoints, this is
  `[script(codepoint)]`.

  ## Examples

      iex> Mojibake.Char.script_extensions(?A)
      [:latin]

      iex> Mojibake.Char.script_extensions(0x3001)
      [:bopomofo, :hangul, :han, :hiragana, :katakana, :mongolian, :yi]
  """
  @spec script_extensions(codepoint()) :: [Script.t()]
  def script_extensions(codepoint) when is_codepoint(codepoint) do
    case Nif.script_extensions(codepoint) do
      {:ok, ids} -> ids |> Enum.map(&Script.from_id/1) |> Enum.reject(&is_nil/1)
      {:error, _} -> [script(codepoint)]
    end
  end

  @doc """
  The East_Asian_Width of `codepoint` (UAX #11): `:ambiguous`, `:fullwidth`, `:halfwidth`,
  `:neutral`, `:narrow`, or `:wide`. Codepoints that `EastAsianWidth.txt` does not list are
  `:neutral`.

  ## Examples

      iex> Mojibake.Char.east_asian_width(?A)
      :narrow

      iex> Mojibake.Char.east_asian_width(0x4E00)
      :wide
  """
  @spec east_asian_width(codepoint()) :: atom()
  def east_asian_width(codepoint) when is_codepoint(codepoint) do
    property(codepoint, :east_asian_width) || :neutral
  end

  @doc """
  The plane of `codepoint`, 0 to 16. See `Mojibake.Plane`.

  ## Examples

      iex> Mojibake.Char.plane(0x1F600)
      1
  """
  @spec plane(codepoint()) :: Mojibake.Plane.t()
  def plane(codepoint) when is_codepoint(codepoint), do: div(codepoint, 0x10000)

  @doc """
  The emoji properties of `codepoint` in `emoji-data.txt`, as a map of booleans:

    * `:emoji` - Emoji
    * `:presentation` - Emoji_Presentation, shown as an emoji by default
    * `:modifier` - Emoji_Modifier, a skin-tone modifier
    * `:modifier_base` - Emoji_Modifier_Base, takes a skin-tone modifier
    * `:component` - Emoji_Component, used inside emoji sequences
    * `:extended_pictographic` - Extended_Pictographic

  `Mojibake.Emoji` classifies whole emoji sequences.

  ## Examples

      iex> Mojibake.Char.emoji_properties(0x1F600)
      %{
        emoji: true,
        presentation: true,
        modifier: false,
        modifier_base: false,
        component: false,
        extended_pictographic: true
      }
  """
  @spec emoji_properties(codepoint()) :: %{
          emoji: boolean(),
          presentation: boolean(),
          modifier: boolean(),
          modifier_base: boolean(),
          component: boolean(),
          extended_pictographic: boolean()
        }
  def emoji_properties(codepoint) when is_codepoint(codepoint) do
    {emoji, presentation, modifier, modifier_base, component, pictographic} =
      case Nif.emoji_properties(codepoint) do
        {:ok, properties} -> properties
        {:error, _} -> {false, false, false, false, false, false}
      end

    %{
      emoji: emoji,
      presentation: presentation,
      modifier: modifier,
      modifier_base: modifier_base,
      component: component,
      extended_pictographic: pictographic
    }
  end

  @doc """
  Return the value of the binary `property` for `codepoint`. See `Mojibake.Property` for the
  properties. Raises `ArgumentError` if `property` is not a binary property.

  ## Examples

      iex> Mojibake.Char.property?(?\\s, :white_space)
      true

      iex> Mojibake.Char.property?(?a, :uppercase)
      false
  """
  @spec property?(codepoint(), Property.t()) :: boolean()
  def property?(codepoint, property) when is_codepoint(codepoint) and is_atom(property) do
    case Property.fetch(property) do
      {:ok, {id, :binary}} ->
        # Noncharacters are invalid arguments to the C library and have no properties.
        match?({:ok, true}, Nif.property(codepoint, id, true))

      _ ->
        raise ArgumentError, "#{inspect(property)} is not a binary property"
    end
  end

  @doc """
  The value of the enumerated `property` for `codepoint`, or `nil` if `codepoint` has none. See
  `Mojibake.Property.values/1` for the values. Raises `ArgumentError` if `property` is not an
  enumerated property.

  ## Examples

      iex> Mojibake.Char.property(?A, :line_break)
      :alphabetic

      iex> Mojibake.Char.property(?,, :word_break)
      :mid_num

      iex> Mojibake.Char.property(0x0627, :joining_type)
      :right_joining
  """
  @spec property(codepoint(), Property.t()) :: atom() | nil
  def property(codepoint, property) when is_codepoint(codepoint) and is_atom(property) do
    case Property.fetch(property) do
      {:ok, {id, values}} when is_tuple(values) ->
        case Nif.property(codepoint, id, false) do
          {:ok, value} when value > 0 and value < tuple_size(values) -> elem(values, value)
          _ -> nil
        end

      _ ->
        raise ArgumentError, "#{inspect(property)} is not an enumerated property"
    end
  end

  @doc """
  Return `true` if `codepoint` is graphic: a letter, mark, number, punctuation, symbol, or space.

  ## Examples

      iex> Mojibake.Char.graphic?(?a)
      true

      iex> Mojibake.Char.graphic?(0x0007)
      false
  """
  @spec graphic?(codepoint()) :: boolean()
  def graphic?(codepoint) when is_codepoint(codepoint), do: Nif.codepoint_is(codepoint, :graphic)

  @doc """
  Return `true` if `codepoint` is a combining character (a mark: Mn, Mc, or Me).

  ## Examples

      iex> Mojibake.Char.combining?(0x0301)
      true
  """
  @spec combining?(codepoint()) :: boolean()
  def combining?(codepoint) when is_codepoint(codepoint),
    do: Nif.codepoint_is(codepoint, :combining)

  @doc """
  Return `true` if `codepoint` is a CJK unified ideograph, in the main block or an extension.

  ## Examples

      iex> Mojibake.Char.cjk_ideograph?(0x4E00)
      true
  """
  @spec cjk_ideograph?(codepoint()) :: boolean()
  def cjk_ideograph?(codepoint) when is_codepoint(codepoint),
    do: Nif.codepoint_is(codepoint, :cjk_ideograph)

  @doc """
  Return `true` if `codepoint` is a CJK unified ideograph in one of the extension blocks.

  ## Examples

      iex> Mojibake.Char.cjk_extension_ideograph?(0x20000)
      true

      iex> Mojibake.Char.cjk_extension_ideograph?(0x4E00)
      false
  """
  @spec cjk_extension_ideograph?(codepoint()) :: boolean()
  def cjk_extension_ideograph?(codepoint) when is_codepoint(codepoint),
    do: Nif.codepoint_is(codepoint, :cjk_extension_ideograph)

  defp to_struct(codepoint, info) do
    {name, category, combining, bidi, decomposition, decimal, digit, numeric, mirrored, upper,
     lower, title} = info

    {numeric_type, numeric_value} = numeric({decimal, digit, numeric})

    %__MODULE__{
      codepoint: codepoint,
      name: name,
      category: @categories |> elem(category) |> elem(0),
      combining_class: combining,
      bidi_class: elem(@bidi_classes, bidi),
      bidi_mirrored: mirrored,
      decomposition_type: elem(UnicodeData.decomposition_types(), decomposition),
      numeric_type: numeric_type,
      numeric_value: numeric_value,
      uppercase: or_self(upper, codepoint),
      lowercase: or_self(lower, codepoint),
      titlecase: or_self(title, codepoint)
    }
  end

  # The C library has no data for these ranges, which the Unicode stability policy fixes.
  defp fallback_category(codepoint) when codepoint in 0xD800..0xDFFF, do: :surrogate

  defp fallback_category(codepoint)
       when codepoint in 0xE000..0xF8FF or codepoint in 0xF0000..0xFFFFD or
              codepoint in 0x100000..0x10FFFD,
       do: :private_use

  defp fallback_category(_codepoint), do: :unassigned

  defp abbreviation(:surrogate), do: "Cs"
  defp abbreviation(:private_use), do: "Co"
  defp abbreviation(:unassigned), do: "Cn"

  defp mapping(codepoint, field) do
    case Nif.codepoint_info(codepoint) do
      {:ok, info} -> info |> elem(field) |> or_self(codepoint)
      {:error, _} -> codepoint
    end
  end

  defp or_self(0, codepoint), do: codepoint
  defp or_self(mapping, _codepoint), do: mapping

  defp numeric({_, _, ""}), do: {nil, nil}
  defp numeric({decimal, _, _}) when decimal >= 0, do: {:decimal, decimal}
  defp numeric({_, digit, _}) when digit >= 0, do: {:digit, digit}

  defp numeric({_, _, value}) do
    case String.split(value, "/") do
      [integer] -> {:numeric, String.to_integer(integer)}
      [numerator, denominator] -> {:numeric, fraction(numerator, denominator)}
    end
  end

  defp fraction(numerator, denominator) do
    {String.to_integer(numerator), String.to_integer(denominator)}
  end
end
