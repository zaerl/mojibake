# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Encoding do
  @moduledoc """
  Unicode encodings: detection, validation, and conversion between UTF-8, UTF-16, and UTF-32.

  An encoding is one of:

    * `:ascii` and `:utf8`
    * `:utf16le`, `:utf16be`, `:utf32le`, and `:utf32be`, with a fixed byte order. A leading
      U+FEFF is text, not a byte-order mark.
    * `:utf16` and `:utf32`, input only: the binary starts with a byte-order mark (BOM), which
      sets the byte order and is not part of the text.

  Errors report byte offsets into the input, like the rest of Mojibake.
  """

  import Mojibake.Internal

  alias Mojibake.{Error, Nif}

  @typedoc "An encoding. See the module documentation."
  @type t :: :ascii | :utf8 | :utf16 | :utf16le | :utf16be | :utf32 | :utf32le | :utf32be

  @encodings [:ascii, :utf8, :utf16, :utf16le, :utf16be, :utf32, :utf32le, :utf32be]

  # mjb_encoding bits
  @ascii 0x1
  @utf8 0x2
  @utf16be 0x8
  @utf16le 0x10
  @utf32 0x20
  @utf32be 0x40
  @utf32le 0x80

  @doc """
  Guess the encoding of `binary`: the one its byte-order mark gives, else `:ascii` or `:utf8` if
  it is well-formed in them. Returns `nil` otherwise, and for an empty binary.

  A BOM gives an explicit byte order, such as `:utf16le`; the BOM is still part of the binary.
  Convert it from `:utf16` or `:utf32` to drop the BOM.

  ## Examples

      iex> Mojibake.Encoding.detect("hello")
      :ascii

      iex> Mojibake.Encoding.detect("héllo")
      :utf8

      iex> Mojibake.Encoding.detect(<<0xFF, 0xFE, ?h, 0>>)
      :utf16le

      iex> Mojibake.Encoding.detect(<<0xFF>>)
      nil
  """
  @spec detect(binary()) :: t() | nil
  def detect(binary) when is_binary(binary) do
    binary |> Nif.detect_encoding() |> from_bits()
  end

  # A UTF-32LE BOM starts with a UTF-16LE BOM; UTF-32 wins.
  defp from_bits(bits) do
    cond do
      Bitwise.band(bits, @utf32) != 0 and Bitwise.band(bits, @utf32le) != 0 -> :utf32le
      Bitwise.band(bits, @utf32be) != 0 -> :utf32be
      Bitwise.band(bits, @utf16le) != 0 -> :utf16le
      Bitwise.band(bits, @utf16be) != 0 -> :utf16be
      Bitwise.band(bits, @ascii) != 0 -> :ascii
      Bitwise.band(bits, @utf8) != 0 -> :utf8
      true -> nil
    end
  end

  @doc """
  Check that `binary` is well-formed in `encoding`. Returns `{:error, %Mojibake.Error{}}` with
  the byte range of the first ill-formed subsequence otherwise.

  ## Examples

      iex> Mojibake.Encoding.validate("héllo", :utf8)
      :ok

      iex> Mojibake.Encoding.validate(<<?a, 0, 0x00, 0xDC>>, :utf16le)
      {:error, %Mojibake.Error{reason: :malformed_input, offset: 2, length: 2, detail: :unpaired_surrogate}}
  """
  @spec validate(binary(), t()) :: :ok | {:error, Error.t()}
  def validate(binary, encoding) when is_binary(binary) and encoding in @encodings do
    case Nif.validate_encoding(binary, encoding) do
      :ok -> :ok
      error -> {:error, Error.from_nif(error)}
    end
  end

  @doc """
  Return `true` if `binary` is well-formed in `encoding`.

  ## Examples

      iex> Mojibake.Encoding.valid?(<<0xC3, 0xA9>>, :utf8)
      true

      iex> Mojibake.Encoding.valid?(<<0xC3, 0xA9>>, :ascii)
      false
  """
  @spec valid?(binary(), t()) :: boolean()
  def valid?(binary, encoding), do: validate(binary, encoding) == :ok

  @doc """
  Convert `binary` from one encoding to another.

  The `:malformed` option says what to do with input that is not well-formed: `:stop` (the
  default) returns an error, `:replace` writes U+FFFD, and `:skip` drops it. Converting to
  `:ascii` returns `{:error, %Mojibake.Error{reason: :unsupported}}` for non-ASCII text.

  ## Examples

      iex> Mojibake.Encoding.convert("hé", :utf8, :utf16be)
      {:ok, <<0, ?h, 0, 0xE9>>}

      iex> Mojibake.Encoding.convert(<<0xFF, 0xFE, ?h, 0, 0xE9, 0>>, :utf16, :utf8)
      {:ok, "hé"}

      iex> Mojibake.Encoding.convert(<<?a, 0xFF>>, :utf8, :utf8, malformed: :replace)
      {:ok, "a\\uFFFD"}
  """
  @spec convert(binary(), t(), t(), keyword()) :: {:ok, binary()} | {:error, Error.t()}
  def convert(binary, from, to, opts \\ [])
      when is_binary(binary) and from in @encodings and to in @encodings and is_list(opts) do
    binary |> Nif.convert_encoding(from, to, malformed_option(opts, :stop)) |> result()
  end

  @doc """
  Like `convert/4`, but raises `Mojibake.Error`.

  ## Examples

      iex> Mojibake.Encoding.convert!(<<0, 0, 0, ?A>>, :utf32be, :utf8)
      "A"
  """
  @spec convert!(binary(), t(), t(), keyword()) :: binary()
  def convert!(binary, from, to, opts \\ []), do: bang(convert(binary, from, to, opts))
end
