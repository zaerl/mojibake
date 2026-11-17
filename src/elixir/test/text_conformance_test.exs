# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.TextConformanceTest do
  # The Unicode conformance tests for collation (UTS #10) and IDNA (UTS #46).
  use ExUnit.Case, async: true

  alias Mojibake.{Collation, IDNA}

  @moduletag :conformance
  @moduletag timeout: :infinity

  defp data_file(path) do
    Path.join(Application.fetch_env!(:mojibake, :test_data_dir), path)
  end

  # The data lines of a test file, without comments, with their line numbers.
  defp lines(path) do
    path
    |> data_file()
    |> File.stream!()
    |> Stream.with_index(1)
    |> Stream.map(fn {line, number} -> {line |> String.split("#") |> hd(), number} end)
    |> Stream.reject(fn {line, _} -> String.trim(line) == "" end)
  end

  # A line such as "0009 0021;" as a string. Like the C tests, drop U+0000 and skip lines with
  # surrogates, which UTF-8 cannot hold; CollationTest.html allows that.
  defp collation_string(line) do
    codepoints =
      line
      |> String.split(";")
      |> hd()
      |> String.split()
      |> Enum.map(&String.to_integer(&1, 16))
      |> Enum.reject(&(&1 == 0))

    if Enum.any?(codepoints, &(&1 in 0xD800..0xDFFF)), do: nil, else: List.to_string(codepoints)
  end

  # Every line must sort after or equal to the one before it.
  defp check_order(path, key) do
    path
    |> lines()
    |> Stream.map(fn {line, number} -> {collation_string(line), number} end)
    |> Stream.reject(fn {string, _} -> string in [nil, ""] end)
    |> Stream.map(fn {string, number} -> {key.(string), number} end)
    |> Stream.chunk_every(2, 1, :discard)
    |> Enum.flat_map(fn [{previous, _}, {current, number}] ->
      if previous > current, do: [number], else: []
    end)
  end

  @tag feature: :collation
  test "CollationTest_NON_IGNORABLE.txt" do
    failures =
      check_order("collation/CollationTest/CollationTest_NON_IGNORABLE.txt", fn string ->
        Collation.sort_key(string,
          variable: :non_ignorable,
          strength: :tertiary,
          malformed: :stop
        )
      end)

    assert failures == []
  end

  @tag feature: :collation
  test "CollationTest_SHIFTED.txt" do
    failures =
      check_order("collation/CollationTest/CollationTest_SHIFTED.txt", fn string ->
        string
        |> Collation.sort_key(variable: :shifted, strength: :quaternary, malformed: :stop)
        |> shifted_test_weights()
      end)

    assert failures == []
  end

  # The Unicode 18 SHIFTED conformance data resets the level-4 weights of variable elements to
  # zero while keeping the FFFF weights of the other elements. Like the C tests, drop the
  # level-4 weights other than 0 and FFFF before comparing; Mojibake itself implements the full
  # UCA level 4.
  defp shifted_test_weights(key) do
    {weights, _} =
      for <<weight::16 <- key>>, reduce: {[], 0} do
        {weights, separators} ->
          separators = if weight == 0, do: separators + 1, else: separators

          if separators >= 3 and weight not in [0, 0xFFFF],
            do: {weights, separators},
            else: {[weight | weights], separators}
      end

    Enum.reverse(weights)
  end

  @tag feature: :idna
  test "IdnaTestV2.txt" do
    {failures, skipped} =
      "idna/IdnaTestV2.txt"
      |> lines()
      |> Enum.map(fn {line, number} -> {String.split(line, ";"), number} end)
      |> Enum.filter(fn {fields, _} -> length(fields) >= 7 end)
      |> Enum.reduce({[], 0}, fn {fields, number}, {failures, skipped} ->
        case idna_case(Enum.map(fields, &String.trim/1)) do
          nil -> {failures, skipped + 1}
          test_case -> {idna_failures(test_case, number) ++ failures, skipped}
        end
      end)

    assert failures == []
    # The two cases with surrogates, which UTF-8 cannot hold.
    assert skipped == 2
  end

  # source; toUnicode; toUnicodeStatus; toAsciiN; toAsciiNStatus; toAsciiT; toAsciiTStatus.
  # An empty toUnicode is the source, an empty toAsciiN is toUnicode, and an empty
  # toAsciiNStatus is toUnicodeStatus.
  defp idna_case([source, unicode, unicode_status, ascii, ascii_status | _]) do
    with {:ok, source} <- idna_field(source),
         {:ok, unicode} <- if(unicode == "", do: {:ok, source}, else: idna_field(unicode)),
         {:ok, ascii} <- if(ascii == "", do: {:ok, unicode}, else: idna_field(ascii)) do
      unicode_error = unicode_status not in ["", "[]"]
      ascii_error = if ascii_status == "", do: unicode_error, else: ascii_status != "[]"

      {source, unicode, unicode_error, ascii, ascii_error}
    else
      :error -> nil
    end
  end

  defp idna_failures({source, unicode, unicode_error, ascii, ascii_error}, number) do
    for {form, expected, expected_error} <- [
          {:unicode, unicode, unicode_error},
          {:ascii, ascii, ascii_error}
        ],
        {converted, errors} = IDNA.process(source, form),
        converted != expected or errors != [] != expected_error do
      {number, form, source, converted, errors}
    end
  end

  # A field with \uXXXX and \x{X} escapes, or "" for the empty string.
  defp idna_field(~S("")), do: {:ok, ""}

  defp idna_field(field) do
    ~r/\\u([0-9A-Fa-f]{4})|\\x\{([0-9A-Fa-f]+)\}/
    |> Regex.split(field, include_captures: true)
    |> Enum.reduce_while({:ok, ""}, fn part, {:ok, acc} ->
      case Regex.run(~r/^\\(?:u([0-9A-Fa-f]{4})|x\{([0-9A-Fa-f]+)\})$/, part) do
        nil ->
          {:cont, {:ok, acc <> part}}

        captures ->
          codepoint = captures |> Enum.drop(1) |> Enum.join() |> String.to_integer(16)

          if codepoint in 0xD800..0xDFFF,
            do: {:halt, :error},
            else: {:cont, {:ok, acc <> <<codepoint::utf8>>}}
      end
    end)
  end
end
