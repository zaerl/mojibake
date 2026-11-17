# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.LanguageTag do
  @moduledoc """
  BCP 47 language tags, such as "en-US", "sr-Latn-RS", or "de-CH-1996".

  `parse/1` checks the syntax of a tag strictly and splits it into its subtags, with the case
  conventions of BCP 47 applied: lowercase language, titlecase script, uppercase region. It does
  not check the subtags against the IANA registry, so "qq-Zzzz" parses.

  `to_string/1` formats a parsed tag back:

      iex> {:ok, tag} = Mojibake.LanguageTag.parse("SR-latn-rs")
      iex> to_string(tag)
      "sr-Latn-RS"

  The `:locale` option of `Mojibake.upcase/2` and the other casing functions takes a tag; only
  its language matters there, and `matched_locale` says which locale Mojibake recognizes.
  """

  import Mojibake.Internal

  alias Mojibake.{Error, Nif}

  @typedoc """
  A parsed language tag:

    * `language` - the language subtag, such as `"sr"`
    * `extlang` - extended language subtags, such as `"yue"` in "zh-yue"
    * `script` - an ISO 15924 script code, such as `"Latn"`
    * `region` - an ISO 3166-1 or UN M.49 region code, such as `"RS"` or `"419"`
    * `variants` - variant subtags, such as `["rozaj", "biske"]`
    * `extensions` - extensions, each with its singleton, such as `["u-ca-buddhist"]`
    * `private_use` - the private-use subtags after "x", such as `["foo"]`
    * `grandfathered` - a whole irregular or grandfathered tag, such as `"i-klingon"`; the other
      fields are then empty
    * `matched_locale` - the ISO 639-1 code of the casing locale Mojibake recognizes in the tag,
      such as `"tr"`, or `nil`

  Absent subtags are `nil` or `[]`.
  """
  @type t :: %__MODULE__{
          language: String.t() | nil,
          extlang: String.t() | nil,
          script: String.t() | nil,
          region: String.t() | nil,
          variants: [String.t()],
          extensions: [String.t()],
          private_use: [String.t()],
          grandfathered: String.t() | nil,
          matched_locale: String.t() | nil
        }

  defstruct language: nil,
            extlang: nil,
            script: nil,
            region: nil,
            variants: [],
            extensions: [],
            private_use: [],
            grandfathered: nil,
            matched_locale: nil

  @doc """
  Parse a BCP 47 language tag. Returns
  `{:error, %Mojibake.Error{reason: :invalid_locale}}` if it is not well-formed.

  ## Examples

      iex> Mojibake.LanguageTag.parse("sl-Latn-IT-rozaj-u-ca-buddhist-x-foo")
      {:ok,
       %Mojibake.LanguageTag{
         language: "sl",
         script: "Latn",
         region: "IT",
         variants: ["rozaj"],
         extensions: ["u-ca-buddhist"],
         private_use: ["foo"],
         matched_locale: "sl"
       }}

      iex> Mojibake.LanguageTag.parse("en--US")
      {:error, %Mojibake.Error{reason: :invalid_locale}}
  """
  @spec parse(String.t()) :: {:ok, t()} | {:error, Error.t()}
  def parse(tag) when is_binary(tag) do
    case Nif.locale_parse(tag) do
      {:ok, fields} -> {:ok, from_fields(fields)}
      {:error, :no_memory} = error -> result(error)
      {:error, _} -> {:error, %Error{reason: :invalid_locale}}
    end
  end

  @doc """
  Like `parse/1`, but raises `Mojibake.Error`.

  ## Examples

      iex> Mojibake.LanguageTag.parse!("tr-TR").matched_locale
      "tr"
  """
  @spec parse!(String.t()) :: t()
  def parse!(tag), do: bang(parse(tag))

  @doc """
  Return `true` if `tag` is a well-formed BCP 47 language tag.

  ## Examples

      iex> Mojibake.LanguageTag.valid?("zh-Hant-TW")
      true

      iex> Mojibake.LanguageTag.valid?("en_US")
      false
  """
  @spec valid?(String.t()) :: boolean()
  def valid?(tag), do: match?({:ok, _}, parse(tag))

  @doc """
  Format a parsed tag.

  ## Examples

      iex> Mojibake.LanguageTag.to_string(%Mojibake.LanguageTag{language: "pt", region: "BR"})
      "pt-BR"
  """
  @spec to_string(t()) :: String.t()
  def to_string(%__MODULE__{grandfathered: grandfathered}) when is_binary(grandfathered),
    do: grandfathered

  def to_string(%__MODULE__{} = tag) do
    private_use = if tag.private_use == [], do: [], else: ["x" | tag.private_use]

    [tag.language, tag.extlang, tag.script, tag.region]
    |> Enum.reject(&is_nil/1)
    |> Kernel.++(tag.variants ++ tag.extensions ++ private_use)
    |> Enum.join("-")
  end

  defp from_fields(fields) do
    {language, extlang, script, region, variant, extensions, private_use, grandfathered, code} =
      fields

    %__MODULE__{
      language: present(language),
      extlang: present(extlang),
      script: present(script),
      region: present(region),
      variants: split(variant),
      extensions: extensions(extensions),
      private_use: private_use |> split() |> Enum.drop(1),
      grandfathered: present(grandfathered),
      matched_locale: present(code)
    }
  end

  defp present(""), do: nil
  defp present(value), do: value

  defp split(""), do: []
  defp split(value), do: String.split(value, "-")

  # "u-ca-buddhist-t-es" -> ["u-ca-buddhist", "t-es"]: each extension starts at a singleton.
  defp extensions(value) do
    value
    |> split()
    |> Enum.chunk_while(
      [],
      fn
        <<_>> = singleton, [] -> {:cont, [singleton]}
        <<_>> = singleton, acc -> {:cont, join(acc), [singleton]}
        subtag, acc -> {:cont, [subtag | acc]}
      end,
      fn
        [] -> {:cont, []}
        acc -> {:cont, join(acc), []}
      end
    )
  end

  defp join(reversed), do: reversed |> Enum.reverse() |> Enum.join("-")

  defimpl String.Chars do
    def to_string(tag), do: Mojibake.LanguageTag.to_string(tag)
  end
end
