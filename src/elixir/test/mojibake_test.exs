# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule MojibakeTest do
  use ExUnit.Case, async: true
  doctest Mojibake

  alias Mojibake.Error

  # Larger than the 16 KiB dirty-scheduler threshold of the NIF.
  @large_size 100_000

  describe "normalize/3" do
    test "all forms" do
      assert Mojibake.normalize("Å", :nfc) == {:ok, "Å"}
      assert Mojibake.normalize("Å", :nfd) == {:ok, "Å"}
      assert Mojibake.normalize("ｶ", :nfkc) == {:ok, "カ"}
      assert Mojibake.normalize("ﬁ", :nfkd) == {:ok, "fi"}
      assert Mojibake.normalize("ÅＢ", :nfkc_casefold) == {:ok, "åb"}
    end

    test "empty input" do
      for form <- [:nfc, :nfd, :nfkc, :nfkd, :nfkc_casefold] do
        assert Mojibake.normalize("", form) == {:ok, ""}
      end
    end

    test "returns normalized input unchanged" do
      input = "already normalized"
      assert {:ok, output} = Mojibake.normalize(input)
      assert output == input
    end

    test "malformed input" do
      assert {:error, %Error{reason: :malformed_input, offset: 3, length: 1, detail: detail}} =
               Mojibake.normalize(<<"abc", 0x80, "d">>)

      assert detail == :unexpected_continuation
      assert Mojibake.normalize(<<"a", 0xFF, "b">>, :nfc, malformed: :replace) == {:ok, "a�b"}
      assert Mojibake.normalize(<<"a", 0xFF, "b">>, :nfc, malformed: :skip) == {:ok, "ab"}
      assert Mojibake.normalize(<<"a", 0xE2, 0x82>>, :nfc, malformed: :replace) == {:ok, "a�"}
    end

    test "embedded NUL" do
      assert Mojibake.normalize("a\0é") == {:ok, "a\0é"}
    end

    test "bang variant raises" do
      assert Mojibake.normalize!("é") == "é"

      assert_raise Error, ~r/malformed input: invalid leading byte at byte 0/, fn ->
        Mojibake.normalize!(<<0xFF>>)
      end
    end

    test "rejects bad arguments" do
      assert_raise FunctionClauseError, fn -> apply(Mojibake, :normalize, ["a", :nfx]) end
      assert_raise FunctionClauseError, fn -> apply(Mojibake, :normalize, [~c"abc"]) end
      assert_raise ArgumentError, fn -> Mojibake.normalize("a", :nfc, malformed: :ignore) end
      assert_raise ArgumentError, fn -> Mojibake.normalize("a", :nfc, unknown: true) end
    end

    test "large input runs on a dirty scheduler" do
      input = String.duplicate("é", div(@large_size, 3))
      assert {:ok, output} = Mojibake.normalize(input)
      assert output == String.duplicate("é", div(@large_size, 3))
      assert Mojibake.normalize!(output, :nfd) == input
    end
  end

  describe "normalized?/2 and quick_check/2" do
    test "forms" do
      assert Mojibake.normalized?("ﬁ", :nfc)
      refute Mojibake.normalized?("ﬁ", :nfkc)
      assert Mojibake.normalized?("strasse", :nfkc_casefold)
      refute Mojibake.normalized?("Straße", :nfkc_casefold)
      assert Mojibake.normalized?("")
    end

    test "malformed input is never normalized" do
      refute Mojibake.normalized?(<<0xFF>>)
      assert {:error, %Error{}} = Mojibake.quick_check(<<0xFF>>, :nfc)
    end

    test "quick check" do
      assert Mojibake.quick_check("Å", :nfd) == {:ok, :no}
      assert Mojibake.quick_check("Å", :nfc) == {:ok, :yes}
    end
  end

  describe "casing" do
    test "full mappings change the length" do
      assert Mojibake.upcase!("ﬃ") == "FFI"
      assert Mojibake.downcase!("İ") == "i̇"
      assert Mojibake.titlecase!("ﬁsh") == "Fish"
    end

    test "locales" do
      assert Mojibake.upcase!("i", locale: "tr") == "İ"
      assert Mojibake.upcase!("i", locale: :az) == "İ"
      assert Mojibake.downcase!("I", locale: "tr-TR") == "ı"
      assert Mojibake.upcase!("i", locale: "en-US") == "I"
      assert Mojibake.upcase!("i", locale: nil) == "I"
      # Lithuanian keeps the dot of i when an accent follows.
      assert Mojibake.downcase!("Ì", locale: "lt") == "i̇̀"
    end

    test "invalid locale" do
      assert Mojibake.upcase("a", locale: "not a tag") ==
               {:error, %Error{reason: :invalid_locale}}

      assert_raise Error, "invalid BCP 47 language tag", fn ->
        Mojibake.upcase!("a", locale: "")
      end

      assert_raise ArgumentError, fn -> Mojibake.upcase("a", locale: 42) end
    end

    test "casefold" do
      assert Mojibake.casefold!("ΣΑΣ") == "σασ"
      assert Mojibake.casefold!("ẞ") == "ss"
      assert Mojibake.casefold!("ẞ", mode: :simple) == "ß"
      assert_raise ArgumentError, fn -> Mojibake.casefold("a", mode: :turkic) end
      assert_raise ArgumentError, fn -> Mojibake.casefold("a", locale: "tr") end
    end

    test "malformed input" do
      assert {:error, %Error{reason: :malformed_input, offset: 1}} =
               Mojibake.upcase(<<"a", 0xC0, 0x80>>)

      assert Mojibake.upcase(<<"a", 0xC0, 0x80>>, malformed: :replace) ==
               {:ok, "A��"}
    end

    test "large input" do
      input = String.duplicate("straße ", div(@large_size, 8))
      assert Mojibake.upcase!(input) == String.duplicate("STRASSE ", div(@large_size, 8))
    end

    test "caseless_equal?/3" do
      assert Mojibake.caseless_equal?("ΣΑΣ", "σας")
      refute Mojibake.caseless_equal?("a", "b")
      assert Mojibake.caseless_equal?("", "")
      assert Mojibake.caseless_equal?("Ⅻ", "xii", :compatibility)
      refute Mojibake.caseless_equal?("Ⅻ", "xii", :canonical)
      assert Mojibake.caseless_equal?("ＡＢＣ", "abc", :identifier)
      assert Mojibake.caseless_equal?(<<0xFF>>, "�")

      assert_raise FunctionClauseError, fn ->
        apply(Mojibake, :caseless_equal?, ["a", "a", :nope])
      end
    end
  end

  describe "segmentation" do
    test "empty input" do
      assert Mojibake.graphemes("") == []
      assert Mojibake.words("") == []
      assert Mojibake.sentences("") == []
      assert Mojibake.line_breaks("") == []
      assert Mojibake.grapheme_count("") == 0
      assert Mojibake.grapheme_prefix("", 3) == ""
    end

    test "segments are sub-binaries that join back into the input" do
      input = "Hello, 世界! Ça va? 👩🏽‍💻 ok."

      for function <- [:graphemes, :words, :sentences] do
        assert input |> then(&apply(Mojibake, function, [&1])) |> Enum.join() == input
      end
    end

    test "Hangul and Indic graphemes" do
      assert Mojibake.graphemes("각각") == ["각", "각"]
      assert Mojibake.graphemes("क्षि") == ["क्षि"]
    end

    test "malformed input is segmented as U+FFFD" do
      assert Mojibake.graphemes(<<"a", 0xFF, 0xFE, "b">>) == ["a", <<0xFF>>, <<0xFE>>, "b"]
      assert Mojibake.grapheme_count(<<"a", 0xFF, "b">>) == 3
      assert Mojibake.codepoint_count(<<0xE2, 0x82>>) == 1
    end

    test "counts" do
      assert Mojibake.grapheme_count("🇮🇹🇫🇷") == 2
      assert Mojibake.word_count("one two, three... 4") == 4
      # UAX #29 has no abbreviation list: it breaks after "Mr." before an uppercase letter.
      assert Mojibake.sentence_count("Mr. Smith arrived. He sat.") == 3
      assert Mojibake.sentence_count("etc. and more.") == 1
      assert Mojibake.codepoint_count("🇮🇹") == 2
    end

    test "prefixes" do
      assert Mojibake.grapheme_prefix("abc", 0) == ""
      assert Mojibake.grapheme_prefix("abc", 10) == "abc"
      assert Mojibake.grapheme_prefix("abc", 2 ** 70) == "abc"
      assert Mojibake.word_prefix("Hello, world!", 1) == "Hello"
      assert_raise FunctionClauseError, fn -> Mojibake.grapheme_prefix("abc", -1) end
    end

    test "line breaks" do
      assert Mojibake.line_breaks("a b") == [allowed: 2, mandatory: 3]
      assert Mojibake.line_breaks("a\r\nb") == [mandatory: 3, mandatory: 4]
      assert Mojibake.line_breaks("日本語") == [allowed: 3, allowed: 6, mandatory: 9]
    end

    test "large input" do
      input = String.duplicate("É a. ", 20_000)
      graphemes = Mojibake.graphemes(input)

      assert length(graphemes) == 20_000 * 5
      assert Enum.join(graphemes) == input
      assert Mojibake.grapheme_count(input) == 20_000 * 5
      assert Mojibake.sentence_count(input) == 20_000
      assert length(Mojibake.words(input)) == 20_000 * 5
      assert length(Mojibake.line_breaks(input)) == 20_000 * 2
    end
  end

  describe "terminal width" do
    test "terminal_width/2" do
      assert Mojibake.terminal_width("") == 0
      assert Mojibake.terminal_width("👍") == 2
      assert Mojibake.terminal_width("ｱ") == 1
      assert Mojibake.terminal_width("α") == 1
      assert Mojibake.terminal_width(<<0xFF>>) == 1
      assert_raise ArgumentError, fn -> Mojibake.terminal_width("a", profile: :wide) end
      assert_raise ArgumentError, fn -> Mojibake.terminal_width("a", unknown: 1) end
    end

    test "truncate_to_width/3" do
      assert Mojibake.truncate_to_width("日本語", 0) == ""
      assert Mojibake.truncate_to_width("日本語", 1) == ""
      assert Mojibake.truncate_to_width("日本語", 100) == "日本語"
      assert Mojibake.truncate_to_width("aéz", 2) == "aé"
      assert Mojibake.truncate_to_width("αβγ", 4, profile: :east_asian) == "αβ"
      assert_raise ArgumentError, fn -> Mojibake.truncate_to_width("a", 1, break: :line) end
    end

    test "large input" do
      input = String.duplicate("日", @large_size)
      assert Mojibake.terminal_width(input) == 2 * @large_size

      assert Mojibake.truncate_to_width(input, 2 * @large_size - 1) |> String.length() ==
               @large_size - 1
    end
  end

  describe "library information" do
    test "features" do
      assert Enum.all?(Mojibake.features(), &(&1 in [:names, :collation, :idna, :security]))
    end

    test "versions" do
      assert Mojibake.version() =~ ~r/^\d+\.\d+\.\d+$/
      assert Mojibake.unicode_version() =~ ~r/^\d+\.\d+\.\d+$/
    end
  end

  describe "concurrency" do
    test "many processes at once" do
      input = String.duplicate("Straße é 日本語. ", 2_000)
      expected = Mojibake.upcase!(input)

      1..50
      |> Task.async_stream(fn _ -> {Mojibake.upcase!(input), Mojibake.graphemes(input)} end,
        max_concurrency: 50
      )
      |> Enum.each(fn {:ok, {upper, graphemes}} ->
        assert upper == expected
        assert Enum.join(graphemes) == input
      end)
    end
  end
end
