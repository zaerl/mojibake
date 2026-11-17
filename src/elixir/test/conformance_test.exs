# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.ConformanceTest do
  # The Unicode conformance tests for normalization (UAX #15), segmentation (UAX #29), and line
  # breaking (UAX #14).
  use ExUnit.Case, async: true

  @moduletag :conformance
  @moduletag timeout: :infinity

  defp data_file(path) do
    Path.join(Application.fetch_env!(:mojibake, :test_data_dir), path)
  end

  # The data lines of a UCD test file, without comments.
  defp lines(path) do
    path
    |> data_file()
    |> File.stream!()
    |> Stream.with_index(1)
    |> Stream.map(fn {line, number} ->
      {line |> String.split("#") |> hd() |> String.trim(), number}
    end)
    |> Stream.reject(fn {line, _} -> line == "" or String.starts_with?(line, "@") end)
  end

  defp codepoints(field) do
    field
    |> String.split()
    |> Enum.map(&String.to_integer(&1, 16))
    |> List.to_string()
  end

  test "NormalizationTest.txt" do
    failures =
      "UCD/NormalizationTest.txt"
      |> lines()
      |> Enum.flat_map(fn {line, number} ->
        [c1, c2, c3, c4, c5] = line |> String.split(";", trim: true) |> Enum.map(&codepoints/1)

        expectations = [
          {:nfc, [c1, c2, c3], c2},
          {:nfc, [c4, c5], c4},
          {:nfd, [c1, c2, c3], c3},
          {:nfd, [c4, c5], c5},
          {:nfkc, [c1, c2, c3, c4, c5], c4},
          {:nfkd, [c1, c2, c3, c4, c5], c5}
        ]

        for {form, sources, expected} <- expectations,
            source <- sources,
            Mojibake.normalize(source, form) != {:ok, expected} or
              not Mojibake.normalized?(expected, form) do
          {number, form, source}
        end
      end)

    assert failures == []
  end

  # Parse a break test line such as "÷ 0041 × 0308 ÷" into the string and the byte offsets of
  # its breaks, without the one at the start.
  defp break_line(line) do
    {string, breaks} =
      line
      |> String.split()
      |> Enum.reduce({"", []}, fn
        "÷", {string, breaks} -> {string, [byte_size(string) | breaks]}
        "×", acc -> acc
        hex, {string, breaks} -> {string <> <<String.to_integer(hex, 16)::utf8>>, breaks}
      end)

    {string, breaks |> Enum.reverse() |> Enum.reject(&(&1 == 0))}
  end

  defp segment_offsets(segments) do
    segments
    |> Enum.scan(0, fn segment, offset -> offset + byte_size(segment) end)
  end

  for {name, function} <- [
        {"GraphemeBreakTest.txt", :graphemes},
        {"WordBreakTest.txt", :words},
        {"SentenceBreakTest.txt", :sentences}
      ] do
    test name do
      failures =
        for {line, number} <- lines("UCD/auxiliary/#{unquote(name)}"),
            {string, expected} = break_line(line),
            segments = apply(Mojibake, unquote(function), [string]),
            segment_offsets(segments) != expected or Enum.join(segments) != string do
          {number, line, segments}
        end

      assert failures == []
    end
  end

  test "LineBreakTest.txt" do
    failures =
      for {line, number} <- lines("UCD/auxiliary/LineBreakTest.txt"),
          {string, expected} = break_line(line),
          breaks = Mojibake.line_breaks(string),
          Enum.map(breaks, &elem(&1, 1)) != expected do
        {number, line, breaks}
      end

    assert failures == []
  end
end
