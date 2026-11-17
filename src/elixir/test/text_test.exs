# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.TextTest do
  # Collation, IDNA, security, identifiers, encodings, and language tags.
  use ExUnit.Case, async: true

  alias Mojibake.{Collation, Encoding, Error, IDNA, Identifier, LanguageTag, Security}

  @features Mojibake.features()

  # Modules that need an optional feature only run their examples with it.
  if :collation in @features, do: doctest(Collation)
  if :idna in @features, do: doctest(IDNA)
  if :security in @features, do: doctest(Security)
  doctest Identifier
  doctest Encoding
  doctest LanguageTag

  # Run `fun` if `feature` is compiled in; otherwise check that it raises :feature_not_enabled.
  defp with_feature(feature, fun) do
    if feature in Mojibake.features() do
      fun.()
    else
      error = assert_raise Error, fun
      assert error.reason == :feature_not_enabled
    end
  end

  describe "Collation" do
    test "strengths and variable weighting" do
      with_feature(:collation, fn ->
        assert Collation.compare("a", "á", strength: :primary) == :eq
        assert Collation.compare("a", "á", strength: :secondary) == :lt
        assert Collation.compare("A", "a", strength: :secondary) == :eq
        assert Collation.compare("A", "a", strength: :tertiary) == :gt
        assert Collation.compare("ab", "a-b") == :gt
        assert Collation.compare("ab", "a-b", variable: :shifted, strength: :tertiary) == :eq
        assert Collation.compare("ab", "a-b", variable: :shifted, strength: :quaternary) != :eq
        assert Collation.compare("", "​", strength: :quaternary) == :eq
      end)
    end

    test "sort keys agree with compare" do
      with_feature(:collation, fn ->
        words = ~w(résumé resume Resume RESUME rèsumé résume a A á b co-op coop CO-OP)

        for opts <- [[], [strength: :primary], [variable: :shifted, strength: :quaternary]],
            first <- words,
            second <- words do
          key_order =
            case {Collation.sort_key(first, opts), Collation.sort_key(second, opts)} do
              {same, same} -> :eq
              {a, b} when a < b -> :lt
              _ -> :gt
            end

          assert key_order == Collation.compare(first, second, opts),
                 "#{inspect(first)} #{inspect(second)} #{inspect(opts)}"
        end

        assert Collation.sort(words) == Enum.sort(words, Collation)
      end)
    end

    test "malformed input" do
      with_feature(:collation, fn ->
        assert Collation.compare(<<0xFF>>, "�") == :eq
        assert Collation.compare(<<?a, 0xFF>>, "a", malformed: :skip) == :eq

        error = assert_raise Error, fn -> Collation.sort_key(<<0xFF>>, malformed: :stop) end
        assert %Error{reason: :malformed_input, offset: 0} = error
      end)
    end

    test "options" do
      assert_raise ArgumentError, fn -> Collation.compare("a", "b", strength: :fifth) end
      assert_raise ArgumentError, fn -> Collation.compare("a", "b", variable: :blanked) end
      assert_raise ArgumentError, fn -> Collation.compare("a", "b", malformed: :ignore) end
    end
  end

  describe "IDNA" do
    test "conversions" do
      with_feature(:idna, fn ->
        assert IDNA.to_ascii!("faß.de") == "xn--fa-hia.de"
        assert IDNA.to_unicode("xn--fa-hia.de") == {:ok, "faß.de"}

        assert {:error, %Error{reason: :invalid_domain, detail: errors}} = IDNA.to_ascii("")
        assert :empty_label in errors

        assert IDNA.process("a_b.example", :ascii) == {"a_b.example", [:std3]}

        long = String.duplicate("a", 64) <> ".example"
        assert {:error, %Error{detail: [:label_length]}} = IDNA.to_ascii(long)
        assert {:ok, _} = IDNA.to_unicode(long)

        error = assert_raise Error, fn -> IDNA.to_ascii!("a..b") end
        assert Exception.message(error) == "invalid domain name: empty label, label length"

        assert {:error, %Error{reason: :malformed_input}} = IDNA.to_ascii(<<0xFF>>)
      end)
    end
  end

  describe "Security" do
    test "confusables and scripts" do
      with_feature(:security, fn ->
        assert Security.confusable?("ѕсоре", "scope")
        refute Security.confusable?("scope", "scape")
        assert Security.confusable?(<<0xFF>>, "�")
        assert Security.skeleton("") == {:ok, ""}
        assert Security.resolved_scripts("") == :all
        assert Security.resolved_scripts("한국어") == [:hangul, :korean]
        assert Security.resolved_scripts("ㄅ中") == [:han_with_bopomofo]
        refute Security.mixed_script?("Ω1")
        assert_raise Error, fn -> Security.resolved_scripts(<<0xFF>>) end
      end)
    end
  end

  test "Identifier" do
    assert Identifier.valid?("変数")
    assert Identifier.valid?("ﬁle", :default)
    refute Identifier.valid?("")
    refute Identifier.valid?(<<?a, 0xFF>>)
    # U+309B KATAKANA-HIRAGANA VOICED SOUND MARK is ID_Start but not XID_Start.
    assert Identifier.start?(0x309B)
    refute Identifier.start?(0x309B, :nfkc)
    assert Identifier.continue?(0x0301, :nfkc)
    refute Identifier.pattern_syntax?(?a)
    refute Identifier.pattern_white_space?(?a)
    assert_raise FunctionClauseError, fn -> apply(Identifier, :valid?, ["a", :strict]) end
  end

  describe "Encoding" do
    test "round trips" do
      text = "héllo, 世界 😀"

      for encoding <- [:utf16le, :utf16be, :utf32le, :utf32be] do
        {:ok, encoded} = Encoding.convert(text, :utf8, encoding)
        assert Encoding.valid?(encoded, encoding)
        assert Encoding.convert!(encoded, encoding, :utf8) == text
        assert encoded == :unicode.characters_to_binary(text, :utf8, erlang(encoding))
      end
    end

    test "byte-order marks" do
      utf16be = <<0xFE, 0xFF, 0, ?h>>
      utf32le = <<0xFF, 0xFE, 0, 0, ?h, 0, 0, 0>>

      assert Encoding.detect(utf16be) == :utf16be
      assert Encoding.detect(utf32le) == :utf32le
      assert Encoding.convert!(utf16be, :utf16, :utf8) == "h"
      assert Encoding.convert!(utf32le, :utf32, :utf8) == "h"
      assert Encoding.convert!(utf16be, :utf16be, :utf8) == "﻿h"
      assert {:error, %Error{reason: :invalid_encoding}} = Encoding.convert("h", :utf8, :utf16)
      assert {:error, %Error{reason: :invalid_encoding}} = Encoding.validate(<<0, ?h>>, :utf16)
    end

    test "malformed input" do
      assert Encoding.detect("") == nil
      assert Encoding.convert(<<0xFF, ?a>>, :utf8, :utf8, malformed: :skip) == {:ok, "a"}

      assert {:error, %Error{reason: :malformed_input, offset: 1, length: 1}} =
               Encoding.convert(<<?a, 0xFF>>, :utf8, :utf16le)

      assert {:error, %Error{reason: :malformed_input, offset: 4, detail: :truncated_code_unit}} =
               Encoding.validate(<<0, ?a, 0, ?b, 0>>, :utf16be)

      assert {:error, %Error{reason: :unsupported}} = Encoding.convert("é", :utf8, :ascii)
      assert_raise FunctionClauseError, fn -> apply(Encoding, :convert, ["a", :utf8, :latin1]) end
    end
  end

  defp erlang(:utf16le), do: {:utf16, :little}
  defp erlang(:utf16be), do: {:utf16, :big}
  defp erlang(:utf32le), do: {:utf32, :little}
  defp erlang(:utf32be), do: {:utf32, :big}

  describe "LanguageTag" do
    test "parse and format" do
      for tag <- ~w(en en-US zh-Hant-TW sr-Latn-RS de-CH-1996 es-419 zh-yue-HK x-private
                    en-a-bbb-x-a-ccc i-klingon art-lojban sl-rozaj-biske-1994
                    en-u-ca-gregory-t-ja-x-foo) do
        assert tag |> LanguageTag.parse!() |> to_string() == tag
      end

      assert LanguageTag.parse!("EN-latn-us") |> to_string() == "en-Latn-US"

      assert %LanguageTag{extensions: ["u-ca-gregory", "t-ja"]} =
               LanguageTag.parse!("en-u-ca-gregory-t-ja-x-foo")

      assert %LanguageTag{grandfathered: "i-klingon", language: nil} =
               LanguageTag.parse!("i-klingon")

      assert %LanguageTag{private_use: ["private"], language: nil} =
               LanguageTag.parse!("x-private")

      assert LanguageTag.parse!("tr").matched_locale == "tr"
      assert LanguageTag.parse!("qq").matched_locale == nil
    end

    test "invalid tags" do
      for tag <- ["", "en-", "-en", "en_US", "toolongtag", "en-US-US", "é"] do
        assert LanguageTag.parse(tag) == {:error, %Error{reason: :invalid_locale}}, tag
        refute LanguageTag.valid?(tag)
      end

      assert_raise Error, "invalid BCP 47 language tag", fn -> LanguageTag.parse!("") end
    end
  end
end
