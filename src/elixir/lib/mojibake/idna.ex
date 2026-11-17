# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.IDNA do
  @moduledoc """
  Internationalized domain names (UTS #46).

  `to_ascii/1` turns a domain name into the ASCII form that DNS uses, encoding non-ASCII labels
  with Punycode. `to_unicode/1` turns it back into Unicode for display. Both map the name first:
  case folding, normalization, and the UTS #46 mapping table, so "BÜCHER.example" and
  "bücher.example" give the same result.

  Processing is nontransitional, with every check on: STD3 ASCII rules, hyphens, CONTEXTJ,
  bidi, and label and domain length.

  These functions need the `:idna` feature. Without it they return or raise `Mojibake.Error` with
  reason `:feature_not_enabled`.
  """

  import Mojibake.Internal

  alias Mojibake.{Error, Nif}

  @typedoc """
  A UTS #46 validation error:

    * `:punycode` - an `xn--` label is not valid Punycode
    * `:empty_label` - a label is empty, as in "a..b"
    * `:hyphen` - a label starts or ends with a hyphen, or has hyphens in positions 3 and 4
    * `:not_nfc` - a label is not in NFC
    * `:leading_mark` - a label starts with a combining mark
    * `:disallowed` - a character is not allowed in domain names
    * `:std3` - a character breaks the STD3 ASCII rules, such as "_"
    * `:contextj` - a ZERO WIDTH JOINER or NON-JOINER is in the wrong context
    * `:bidi` - the name breaks the bidi rules of RFC 5893
    * `:label_length` - a label is longer than 63 bytes in ASCII form
    * `:domain_length` - the name is longer than 253 bytes in ASCII form
  """
  @type error ::
          :punycode
          | :empty_label
          | :hyphen
          | :not_nfc
          | :leading_mark
          | :disallowed
          | :std3
          | :contextj
          | :bidi
          | :label_length
          | :domain_length

  # In the bit order of mjb_idna_error.
  @errors [
    :punycode,
    :empty_label,
    :hyphen,
    :not_nfc,
    :leading_mark,
    :disallowed,
    :std3,
    :contextj,
    :bidi,
    :label_length,
    :domain_length
  ]

  @doc """
  Convert `domain` to its ASCII form.

  Returns `{:error, %Mojibake.Error{reason: :invalid_domain, detail: errors}}` with the list of
  `t:error/0` if the name is not valid. Do not look up an invalid name.

  ## Examples

      iex> Mojibake.IDNA.to_ascii("Bücher.example")
      {:ok, "xn--bcher-kva.example"}

      iex> {:error, error} = Mojibake.IDNA.to_ascii("-bad-.example")
      iex> error.detail
      [:hyphen]
  """
  @spec to_ascii(String.t()) :: {:ok, String.t()} | {:error, Error.t()}
  def to_ascii(domain) when is_binary(domain), do: convert(domain, :ascii)

  @doc """
  Like `to_ascii/1`, but raises `Mojibake.Error`.

  ## Examples

      iex> Mojibake.IDNA.to_ascii!("例え.テスト")
      "xn--r8jz45g.xn--zckzah"
  """
  @spec to_ascii!(String.t()) :: String.t()
  def to_ascii!(domain), do: bang(to_ascii(domain))

  @doc """
  Convert `domain` to its Unicode form, decoding Punycode labels.

  Returns `{:error, %Mojibake.Error{reason: :invalid_domain, detail: errors}}` if the name is not
  valid. `process/2` returns the converted name even then.

  ## Examples

      iex> Mojibake.IDNA.to_unicode("xn--bcher-kva.example")
      {:ok, "bücher.example"}

      iex> Mojibake.IDNA.to_unicode("WWW.Example.COM")
      {:ok, "www.example.com"}
  """
  @spec to_unicode(String.t()) :: {:ok, String.t()} | {:error, Error.t()}
  def to_unicode(domain) when is_binary(domain), do: convert(domain, :unicode)

  @doc """
  Like `to_unicode/1`, but raises `Mojibake.Error`.

  ## Examples

      iex> Mojibake.IDNA.to_unicode!("xn--r8jz45g.xn--zckzah")
      "例え.テスト"
  """
  @spec to_unicode!(String.t()) :: String.t()
  def to_unicode!(domain), do: bang(to_unicode(domain))

  @doc """
  Process `domain` into its `:ascii` or `:unicode` form and return the result with the list of
  validation errors, which is empty for a valid name. The result is a best effort when there are
  errors: show it to people if you like, but never look it up.

  Raises `Mojibake.Error` if `domain` is not well-formed UTF-8.

  ## Examples

      iex> Mojibake.IDNA.process("a..Example", :unicode)
      {"a..example", [:empty_label]}

      iex> Mojibake.IDNA.process("ok.example", :ascii)
      {"ok.example", []}
  """
  @spec process(String.t(), :ascii | :unicode) :: {String.t(), [error()]}
  def process(domain, form) when is_binary(domain) and form in [:ascii, :unicode] do
    {converted, bits} = domain |> Nif.idna(form) |> value!()

    {converted, errors(bits)}
  end

  defp convert(domain, form) do
    case Nif.idna(domain, form) do
      {:ok, {converted, 0}} ->
        {:ok, converted}

      {:ok, {_, bits}} ->
        {:error, %Error{reason: :invalid_domain, detail: errors(bits)}}

      error ->
        result(error)
    end
  end

  defp errors(bits) do
    for {error, index} <- Enum.with_index(@errors),
        Bitwise.band(bits, Bitwise.bsl(1, index)) != 0,
        do: error
  end
end
