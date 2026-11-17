# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Script do
  @moduledoc """
  Unicode scripts (UAX #24).

  A script is an atom made from the long name of its `Script` property value, such as `:latin`,
  `:greek`, `:han`, `:common` (characters shared by many scripts), `:inherited` (combining
  marks that take the script of their base), or `:unknown` (unassigned codepoints). Three
  ISO 15924 combinations used by the identifier and confusable checks come last: `:han_with_bopomofo`,
  `:japanese`, and `:korean`.

  `Mojibake.Char.script/1` and `Mojibake.Char.script_extensions/1` return scripts.
  """

  alias Mojibake.UnicodeData

  @typedoc "A script, such as `:latin`."
  @type t :: atom()

  @scripts UnicodeData.scripts() |> Tuple.to_list() |> tl()
  @atoms Enum.map(@scripts, &elem(&1, 0))
  @codes Map.new(@scripts)
  @from_code Map.new(@scripts, fn {script, code} -> {String.downcase(code), script} end)

  @doc """
  Every script, in the order of the `Script` property values, then the three ISO 15924
  combinations.

  ## Examples

      iex> :latin in Mojibake.Script.all()
      true
  """
  @spec all() :: [t()]
  def all, do: @atoms

  @doc """
  The ISO 15924 code of `script`, or `nil` if `script` is not a script.

  ## Examples

      iex> Mojibake.Script.code(:latin)
      "Latn"

      iex> Mojibake.Script.code(:japanese)
      "Jpan"
  """
  @spec code(t()) :: String.t() | nil
  def code(script) when is_atom(script), do: Map.get(@codes, script)

  @doc """
  The script with the ISO 15924 code `code`, ignoring case, or `nil` if there is none.

  ## Examples

      iex> Mojibake.Script.from_code("Grek")
      :greek

      iex> Mojibake.Script.from_code("zyyy")
      :common

      iex> Mojibake.Script.from_code("Xxxx")
      nil
  """
  @spec from_code(String.t()) :: t() | nil
  def from_code(code) when is_binary(code), do: Map.get(@from_code, String.downcase(code))

  @doc false
  # The script for a mjb_script value; nil for MJB_SC_NOT_SET or an unknown value.
  def from_id(id) when is_integer(id) and id > 0 and id <= length(@scripts) do
    UnicodeData.scripts() |> elem(id) |> elem(0)
  end

  def from_id(_id), do: nil
end
