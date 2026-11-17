# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Plane do
  @moduledoc """
  Unicode planes: the 17 ranges of 65,536 codepoints, numbered 0 to 16.

  `Mojibake.Char.plane/1` returns the plane of a codepoint.
  """

  alias Mojibake.Nif

  @typedoc "A plane number, 0 to 16."
  @type t :: 0..16

  @doc """
  The name of `plane`, or `nil` for the unassigned planes 4 to 13.

  ## Examples

      iex> Mojibake.Plane.name(0)
      "Basic Multilingual Plane"

      iex> Mojibake.Plane.name(5)
      nil
  """
  @spec name(t()) :: String.t() | nil
  def name(plane) when plane in 0..16, do: plane |> Nif.plane_name(false) |> named()

  @doc """
  The abbreviation of `plane`, such as `"BMP"`, or `nil` for the unassigned planes 4 to 13.

  ## Examples

      iex> Mojibake.Plane.abbreviation(1)
      "SMP"

      iex> Mojibake.Plane.abbreviation(15)
      "PUA-A"
  """
  @spec abbreviation(t()) :: String.t() | nil
  def abbreviation(plane) when plane in 0..16, do: plane |> Nif.plane_name(true) |> named()

  @doc """
  The codepoints of `plane`.

  ## Examples

      iex> Mojibake.Plane.range(1)
      65536..131071
  """
  @spec range(t()) :: Range.t()
  def range(plane) when plane in 0..16, do: (plane * 0x10000)..(plane * 0x10000 + 0xFFFF)//1

  defp named("Unassigned"), do: nil
  defp named(name), do: name
end
