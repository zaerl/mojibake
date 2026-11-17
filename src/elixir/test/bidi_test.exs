# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.BidiTest do
  use ExUnit.Case, async: true

  alias Mojibake.{Bidi, Error}
  alias Mojibake.Bidi.Paragraph

  doctest Bidi

  describe "resolve/2" do
    test "an empty paragraph" do
      paragraph = Bidi.resolve!("")

      assert %Paragraph{text: "", direction: :ltr, level: 0, length: 0} = paragraph
      assert Bidi.chars(paragraph) == []
      assert Bidi.reorder(paragraph) == []
      assert Bidi.runs(paragraph) == []
      assert Bidi.visual(paragraph) == ""
      assert Bidi.direction("") == :ltr
    end

    test "a forced direction overrides the first strong character" do
      assert Bidi.resolve!("abc", :rtl).direction == :rtl
      assert Bidi.resolve!("אבג", :ltr).direction == :ltr
      assert Bidi.resolve!("123 abc", :auto).direction == :ltr
    end

    test "rule X9 removes embedding controls and Boundary Neutrals" do
      # a RLE b PDF ZWJ c
      text = "a\u202Bb\u202C\u200Dc"
      paragraph = Bidi.resolve!(text)

      assert paragraph.length == 3

      assert [
               %{codepoint: ?a, offset: 0},
               %{codepoint: ?b, offset: 4},
               %{codepoint: ?c, offset: 11}
             ] =
               Bidi.chars(paragraph)

      assert Bidi.visual(paragraph) == "abc"
    end

    test "malformed input and bad arguments" do
      assert {:error, %Error{reason: :malformed_input, offset: 3, length: 1}} =
               Bidi.resolve("abc" <> <<0x80>>)

      assert_raise Error, fn -> Bidi.visual(<<0xFF>>) end
      # Built at runtime, so that the compiler does not flag the bad calls.
      bad = String.to_atom("up")
      assert_raise FunctionClauseError, fn -> Bidi.resolve("abc", bad) end
    end
  end

  describe "lines" do
    setup do
      %{paragraph: Bidi.resolve!("abc אבג")}
    end

    test "a line is a range of positions", %{paragraph: paragraph} do
      assert Bidi.reorder(paragraph, 0..6) == Bidi.reorder(paragraph)
      assert Bidi.reorder(paragraph, 3..5) == [3, 5, 4]
      assert Bidi.reorder(paragraph, 2..1//1) == []
      assert Bidi.visual(paragraph, 6..6) == "ג"

      assert Bidi.runs(paragraph, 2..5) == [
               %{visual: 0..1, logical: 2..3, level: 0, direction: :ltr},
               %{visual: 2..3, logical: 4..5, level: 1, direction: :rtl}
             ]
    end

    test "lines outside the paragraph raise", %{paragraph: paragraph} do
      for line <- [0..7, -1..2, 5..3//-1, 0..6//2, 8..7//1, :all, [0, 1]] do
        assert_raise ArgumentError, fn -> Bidi.reorder(paragraph, line) end
        assert_raise ArgumentError, fn -> Bidi.runs(paragraph, line) end
        assert_raise ArgumentError, fn -> Bidi.visual(paragraph, line) end
      end
    end

    test "trailing whitespace takes the paragraph level (rule L1)" do
      paragraph = Bidi.resolve!("אבג  ", :ltr)

      assert Bidi.levels(paragraph) == [1, 1, 1, 0, 0]
      assert Bidi.reorder(paragraph) == [2, 1, 0, 3, 4]
    end
  end

  test "mirrored characters in right-to-left runs (rule L4)" do
    paragraph = Bidi.resolve!("א(ב)[ג]")

    assert paragraph |> Bidi.chars() |> Enum.map(& &1.mirror) == [nil, ?), nil, ?(, ?], nil, ?[]

    assert Bidi.visual(paragraph) == "[ג](ב)א"
  end

  test "numbers in right-to-left text" do
    paragraph = Bidi.resolve!("מחיר 120 ש״ח")

    assert Bidi.visual(paragraph) == "ח״ש 120 ריחמ"

    assert paragraph |> Bidi.runs() |> Enum.map(& &1.level) == [1, 2, 1]
  end

  test "a paragraph can be shared between processes" do
    paragraph = Bidi.resolve!("abc אבג")

    results =
      1..8
      |> Enum.map(fn _ -> Task.async(fn -> Bidi.visual(paragraph) end) end)
      |> Task.await_many()

    assert Enum.uniq(results) == ["abc גבא"]
  end

  test "long paragraphs run on dirty schedulers" do
    text = String.duplicate("abc אבג ", 5000)
    paragraph = Bidi.resolve!(text)

    assert paragraph.length == 40_000
    assert length(Bidi.levels(paragraph)) == 40_000
    assert byte_size(Bidi.visual(paragraph)) == byte_size(text)
    # The trailing space is a run of its own at the paragraph level (rule L1).
    assert length(Bidi.runs(paragraph)) == 10_001
  end
end
