# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.UCDTest do
  # Check the character data and its atoms against the Unicode Character Database files that
  # utils/generate/scripts/download-test-data.sh downloads.
  use ExUnit.Case, async: true

  alias Mojibake.{Block, Char, Property, Script}

  @moduletag :conformance
  @moduletag timeout: :infinity

  @bidi_classes %{
    "L" => :left_to_right,
    "R" => :right_to_left,
    "AL" => :arabic_letter,
    "EN" => :european_number,
    "ES" => :european_separator,
    "ET" => :european_terminator,
    "AN" => :arabic_number,
    "CS" => :common_separator,
    "NSM" => :nonspacing_mark,
    "BN" => :boundary_neutral,
    "B" => :paragraph_separator,
    "S" => :segment_separator,
    "WS" => :white_space,
    "ON" => :other_neutral,
    "LRE" => :left_to_right_embedding,
    "LRO" => :left_to_right_override,
    "RLE" => :right_to_left_embedding,
    "RLO" => :right_to_left_override,
    "PDF" => :pop_directional_format,
    "LRI" => :left_to_right_isolate,
    "RLI" => :right_to_left_isolate,
    "FSI" => :first_strong_isolate,
    "PDI" => :pop_directional_isolate
  }

  # The enumerated properties, their PropertyValueAliases.txt names, and their data files.
  @enumerated [
    {:east_asian_width, "ea", "UCD/EastAsianWidth.txt"},
    {:line_break, "lb", "UCD/LineBreak.txt"},
    {:grapheme_cluster_break, "GCB", "UCD/auxiliary/GraphemeBreakProperty.txt"},
    {:word_break, "WB", "UCD/auxiliary/WordBreakProperty.txt"},
    {:sentence_break, "SB", "UCD/auxiliary/SentenceBreakProperty.txt"},
    {:script, "sc", "UCD/Scripts.txt"},
    {:joining_type, "jt", "UCD/extracted/DerivedJoiningType.txt"},
    {:indic_conjunct_break, "InCB", "UCD/DerivedCoreProperties.txt"}
  ]

  @binary_files ["UCD/PropList.txt", "UCD/DerivedCoreProperties.txt", "UCD/emoji/emoji-data.txt"]

  defp data_file(path) do
    Path.join(Application.fetch_env!(:mojibake, :test_data_dir), path)
  end

  # The data fields of each line of a UCD file, without comments.
  defp records(path) do
    path
    |> data_file()
    |> File.stream!()
    |> Stream.map(&(&1 |> String.split("#") |> hd() |> String.trim()))
    |> Stream.reject(&(&1 == ""))
    |> Stream.map(fn line -> line |> String.split(";") |> Enum.map(&String.trim/1) end)
  end

  defp range(field) do
    case String.split(field, "..") do
      [first, last] -> String.to_integer(first, 16)..String.to_integer(last, 16)//1
      [single] -> String.to_integer(single, 16)..String.to_integer(single, 16)//1
    end
  end

  defp noncharacter?(codepoint) do
    codepoint in 0xFDD0..0xFDEF or Bitwise.band(codepoint, 0xFFFE) == 0xFFFE
  end

  defp hex(""), do: nil
  defp hex(field), do: String.to_integer(field, 16)

  test "UnicodeData.txt" do
    names? = :names in Mojibake.features()

    # Mojibake reports the corrected names of NameAliases.txt.
    corrections =
      for [code, name, "correction"] <- Enum.to_list(records("UCD/NameAliases.txt")),
          into: %{},
          do: {code, name}

    failures =
      "UCD/UnicodeData.txt"
      |> records()
      |> Stream.reject(fn [_, name | _] -> String.ends_with?(name, ", First>") end)
      |> Stream.reject(fn [_, name | _] -> String.ends_with?(name, ", Last>") end)
      |> Enum.flat_map(fn fields ->
        [
          code,
          original_name,
          category,
          ccc,
          bidi,
          decomposition,
          _,
          _,
          numeric,
          mirrored,
          _,
          _ | rest
        ] =
          fields

        [upper, lower, title] = Enum.map(rest, &hex/1)
        codepoint = String.to_integer(code, 16)
        char = Char.info(codepoint)
        name = Map.get(corrections, code, original_name)

        decomposition_type =
          case decomposition do
            "" -> nil
            "<" <> tag -> tag |> String.trim_trailing(">") |> String.split(">") |> hd()
            _ -> "canonical"
          end

        expected = [
          name: if(String.starts_with?(name, "<"), do: char && char.name, else: name),
          category: category,
          combining_class: String.to_integer(ccc),
          bidi_class: Map.fetch!(@bidi_classes, bidi),
          bidi_mirrored: mirrored == "Y",
          decomposition_type: decomposition_type && Macro.underscore(decomposition_type),
          numeric_value: numeric,
          uppercase: upper || codepoint,
          lowercase: lower || codepoint,
          titlecase: title || upper || codepoint
        ]

        actual =
          char &&
            [
              name: char.name,
              category: Char.category_abbreviation(codepoint),
              combining_class: char.combining_class,
              bidi_class: char.bidi_class,
              bidi_mirrored: char.bidi_mirrored,
              decomposition_type:
                char.decomposition_type && Atom.to_string(char.decomposition_type),
              numeric_value: numeric_string(char.numeric_value),
              uppercase: char.uppercase,
              lowercase: char.lowercase,
              titlecase: char.titlecase
            ]

        expected = if names?, do: expected, else: Keyword.delete(expected, :name)
        actual = if names? or actual == nil, do: actual, else: Keyword.delete(actual, :name)

        if actual == expected, do: [], else: [{code, expected, actual}]
      end)

    assert Enum.take(failures, 10) == []
  end

  defp numeric_string(nil), do: ""
  defp numeric_string({numerator, denominator}), do: "#{numerator}/#{denominator}"
  defp numeric_string(integer), do: Integer.to_string(integer)

  test "Blocks.txt" do
    blocks = "UCD/Blocks.txt" |> records() |> Enum.to_list()

    assert length(blocks) == length(Block.all())

    for {[field, name], block} <- Enum.zip(blocks, Block.all()) do
      range = range(field)

      assert Block.name(block) == name
      assert Block.range(block) == range
      assert Char.block(range.first) == block
      assert Char.block(range.last) == block or noncharacter?(range.last)
    end
  end

  # PropertyValueAliases.txt maps every value name to its position, which is the position of its
  # atom in Mojibake.Property.values/1.
  defp value_atoms(property, short_name) do
    atoms = Property.values(property)

    names =
      "UCD/PropertyValueAliases.txt"
      |> records()
      |> Enum.filter(&(hd(&1) == short_name))
      |> Enum.map(&tl/1)

    assert length(names) == length(atoms), "#{property} has #{length(names)} values"

    for {aliases, atom} <- Enum.zip(names, atoms), name <- aliases, into: %{}, do: {name, atom}
  end

  # DerivedCoreProperties.txt has the InCB values among its binary properties.
  defp value_record?([_, "InCB", _], property), do: property == :indic_conjunct_break
  defp value_record?([_, _], property), do: property != :indic_conjunct_break
  defp value_record?(_, _), do: false

  for {property, short_name, path} <- @enumerated do
    test "#{property} in #{path}" do
      property = unquote(property)
      atoms = value_atoms(property, unquote(short_name))

      failures =
        unquote(path)
        |> records()
        |> Stream.filter(&value_record?(&1, property))
        |> Enum.flat_map(fn fields ->
          value = Map.fetch!(atoms, List.last(fields))

          for codepoint <- range(hd(fields)),
              not noncharacter?(codepoint),
              Char.property(codepoint, property) != value,
              do: {codepoint, value}
        end)

      assert Enum.take(failures, 10) == []
    end
  end

  test "script codes" do
    for [property, code, long | _] <- Enum.to_list(records("UCD/PropertyValueAliases.txt")),
        property == "sc" do
      script = Script.from_code(code)
      squash = &(&1 |> String.downcase() |> String.replace("_", ""))

      assert squash.(Atom.to_string(script)) == squash.(long)
      assert Script.code(script) == code
    end
  end

  test "binary properties" do
    listed =
      @binary_files
      |> Enum.flat_map(&Enum.to_list(records(&1)))
      |> Enum.filter(&match?([_, _], &1))
      |> Enum.group_by(
        fn [_, name] -> name |> String.downcase() |> String.to_atom() end,
        fn [field, _] -> range(field) end
      )

    binary = Enum.filter(Property.all(), &Property.binary?/1)

    # Every binary property comes from one of the files.
    assert binary -- Map.keys(listed) == []

    failures =
      Enum.flat_map(binary, fn property ->
        ranges = Map.fetch!(listed, property)
        members = MapSet.new(Enum.flat_map(ranges, &Enum.to_list/1))

        missing =
          for codepoint <- members,
              not noncharacter?(codepoint),
              not Char.property?(codepoint, property),
              do: {property, codepoint, true}

        # Every 7th codepoint is enough to catch a wrong property id.
        extra =
          for codepoint <- 0..0x10FFFF//7,
              not MapSet.member?(members, codepoint),
              Char.property?(codepoint, property),
              do: {property, codepoint, false}

        missing ++ extra
      end)

    assert Enum.take(failures, 10) == []
  end
end
