# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.CharTest do
  use ExUnit.Case, async: true

  alias Mojibake.{Block, Char, Emoji, Hangul, Nif, Plane, Property, Script, UnicodeData}

  @names :names in Mojibake.features()

  # The examples that print character names need the :names feature.
  doctest Char, except: if(@names, do: [], else: [name: 1, info: 1])
  doctest Script
  doctest Block
  doctest Property
  doctest Plane
  doctest Emoji
  doctest Hangul

  test "the generated tables match the C enums" do
    limits = Nif.limits()

    assert tuple_size(UnicodeData.categories()) == limits.categories
    assert tuple_size(UnicodeData.blocks()) == limits.blocks
    assert tuple_size(UnicodeData.scripts()) == limits.scripts
    assert Property.values(:east_asian_width) |> length() == limits.east_asian_widths - 1

    for {_, {id, _}} <- UnicodeData.properties() do
      assert id >= 0 and id < limits.properties
    end
  end

  describe "info/1" do
    test "assigned codepoints" do
      info = Char.info(?A)

      assert %Char{codepoint: ?A, category: :uppercase_letter, lowercase: ?a} = info
      assert info.uppercase == ?A
      assert info.titlecase == ?A
      assert info.bidi_class == :left_to_right
      assert info.decomposition_type == nil
      assert info.numeric_type == nil

      assert %Char{numeric_type: :decimal, numeric_value: 5} = Char.info(?5)
      assert %Char{combining_class: 230, category: :nonspacing_mark} = Char.info(0x0301)
      assert %Char{decomposition_type: :canonical} = Char.info(?é)
    end

    test "ranges in UnicodeData.txt" do
      assert %Char{category: :other_letter, bidi_class: :left_to_right} = Char.info(0x4E00)
      assert %Char{category: :other_letter} = Char.info(0xAC00)
      # The C library has no UnicodeData.txt record for these.
      assert Char.info(0xE000) == nil
      assert Char.info(0xD800) == nil
      assert Char.category(0xE000) == :private_use
      assert Char.category(0x10FFFD) == :private_use
      assert Char.category_abbreviation(0xDFFF) == "Cs"
      assert Char.category(0xFFFF) == :unassigned
    end

    test "unassigned codepoints" do
      assert Char.info(0x0378) == nil
      assert Char.info(0x10FFFF) == nil
      assert Char.category(0x10FFFF) == :unassigned
      assert Char.category_abbreviation(0x0378) == "Cn"
      assert Char.combining_class(0x0378) == 0
      assert Char.bidi_class(0x0378) == nil
      assert Char.simple_upcase(0x0378) == 0x0378
    end

    test "names" do
      if @names do
        assert Char.info(0x1F600).name == "GRINNING FACE"
        assert Char.name(0xAC00) == "HANGUL SYLLABLE GA"
        assert Char.name(0x0378) == nil
      else
        assert Char.info(?A).name == "Codepoint U+0041"
        assert_raise Mojibake.Error, fn -> Char.name(?A) end
      end
    end
  end

  test "numeric values" do
    assert Char.numeric_value(0x0F33) == {-1, 2}
    assert Char.numeric_value(0x16B61) == 1_000_000_000_000
    assert Char.numeric_type(0x16B61) == :numeric
    assert Char.numeric_value(?a) == nil
    assert Char.numeric_type(?a) == nil
  end

  test "case mappings" do
    assert Char.simple_upcase(?ÿ) == ?Ÿ
    assert Char.simple_downcase(?İ) == ?i
    assert Char.simple_titlecase(?a) == ?A
    assert Char.simple_titlecase(?ǅ) == ?ǅ
    assert Char.simple_upcase(?1) == ?1
  end

  test "blocks, scripts and planes" do
    assert Char.block(0x10FFFD) == :supplementary_private_use_area_b
    assert Char.block(0xFFFF) == nil
    assert Char.block(0x2FE0) == nil
    assert Char.script(0x0301) == :inherited
    assert Char.script(0x0378) == :unknown
    assert Char.script_extensions(0x0378) == [:unknown]
    assert Char.plane(0x10FFFF) == 16
    assert Plane.name(16) == "Supplementary Private Use Area-B"
    assert Plane.abbreviation(4) == nil
    assert Plane.range(16) == 0x100000..0x10FFFF
    assert Block.name(:nope) == nil
    assert Block.range(:nope) == nil
    assert Script.code(:nope) == nil
    assert length(Script.all()) == tuple_size(UnicodeData.scripts()) - 1
    assert Enum.all?(Script.all(), &(&1 |> Script.code() |> Script.from_code() == &1))
  end

  describe "properties" do
    test "binary" do
      assert Char.property?(?a, :alphabetic)
      assert Char.property?(0x1F600, :emoji)
      refute Char.property?(?a, :emoji)
      assert Char.property?(0x200D, :join_control)
      refute Char.property?(0x10FFFF, :alphabetic)
      assert Char.property(0xFFFF, :line_break) == nil
      assert Char.script_extensions(0xFFFF) == [:unknown]
      refute Char.emoji_properties(0xFFFF).emoji
    end

    test "enumerated" do
      assert Char.property(?\n, :line_break) == :line_feed
      assert Char.property(?a, :word_break) == :a_letter
      assert Char.property(0x1F1E6, :grapheme_cluster_break) == :regional_indicator
      assert Char.property(?a, :grapheme_cluster_break) == nil
      assert Char.property(0x094D, :indic_conjunct_break) == :linker
      assert Char.property(?., :sentence_break) == :a_term
      assert Char.property(?a, :script) == :latin
      assert Char.east_asian_width(0xFF21) == :fullwidth
      assert Char.east_asian_width(0x100000) == :ambiguous
    end

    test "every property is queryable" do
      for property <- Property.all() do
        if Property.binary?(property) do
          assert is_boolean(Char.property?(?a, property))
        else
          values = Property.values(property)
          assert Enum.all?(values, &is_atom/1)
          value = Char.property(?a, property)
          assert value == nil or value in values
        end
      end
    end

    test "wrong properties" do
      assert_raise ArgumentError, fn -> Char.property?(?a, :line_break) end
      assert_raise ArgumentError, fn -> Char.property?(?a, :nope) end
      assert_raise ArgumentError, fn -> Char.property(?a, :alphabetic) end
      assert Property.values(:alphabetic) == nil
      refute Property.binary?(:nope)
    end
  end

  test "emoji properties" do
    assert Char.emoji_properties(?a) == %{
             emoji: false,
             presentation: false,
             modifier: false,
             modifier_base: false,
             component: false,
             extended_pictographic: false
           }

    assert %{emoji: true, component: true, presentation: false} = Char.emoji_properties(?#)
    assert %{modifier: true} = Char.emoji_properties(0x1F3FD)
    assert %{modifier_base: true} = Char.emoji_properties(0x1F44D)
  end

  test "emoji sequences" do
    assert %{type: :keycap, qualification: :fully_qualified} = Emoji.sequence("1\uFE0F\u20E3")
    assert %{type: nil, qualification: :unqualified} = Emoji.sequence("1\u20E3")
    refute Emoji.rgi?("1\u20E3")
    assert %{type: :zwj} = Emoji.sequence("\u{1F468}\u200D\u{1F469}\u200D\u{1F467}")
    assert %{type: :basic, qualification: :fully_qualified} = Emoji.sequence("\u2764\uFE0F")
    assert %{type: :text_variation} = Emoji.sequence("\u2764\uFE0E")
    refute Emoji.rgi?("\u2764\uFE0E")

    assert %{type: :tag} =
             Emoji.sequence("\u{1F3F4}\u{E0067}\u{E0062}\u{E0073}\u{E0063}\u{E0074}\u{E007F}")

    assert Emoji.sequence("") == nil
    assert Emoji.sequence(<<0xFF>>) == nil
  end

  test "Hangul" do
    assert Hangul.compose(0x1100, 0x1161, 0x11A7) == nil
    assert Hangul.compose(0x1100, 0x1176) == nil
    assert Hangul.syllable_name(?a) == nil
    assert Hangul.syllable_name(0xD7A3) == "HANGUL SYLLABLE HIH"
    assert Hangul.leading_jamo?(0x1112)
    refute Hangul.leading_jamo?(0x1113)
    assert Hangul.vowel_jamo?(0x1175)
    assert Hangul.trailing_jamo?(0x11C2)
    refute Hangul.syllable?(0xD7A4)

    for syllable <- [0xAC00, 0xAC01, 0xD7A3] do
      {l, v, t} = Hangul.decompose(syllable)
      assert Hangul.compose(l, v, t) == syllable
    end
  end

  test "arguments that are not codepoints" do
    for bad <- [-1, 0x110000, "a", 1.0] do
      assert_raise FunctionClauseError, fn -> apply(Char, :info, [bad]) end
      assert_raise FunctionClauseError, fn -> apply(Char, :category, [bad]) end
    end

    assert_raise FunctionClauseError, fn -> apply(Plane, :name, [17]) end
    assert_raise FunctionClauseError, fn -> apply(Emoji, :sequence, [?a]) end
  end
end
