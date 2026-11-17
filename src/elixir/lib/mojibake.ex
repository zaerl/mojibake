# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake do
  @moduledoc """
  Unicode text processing for Elixir, backed by the Mojibake C library.

  Mojibake adds what `String` and `:unicode` leave out, with Unicode data that does not
  depend on the OTP release:

    * normalization to NFC, NFD, NFKC, NFKD, and NFKC_Casefold, with quick checks (UAX #15)
    * locale-aware upper, lower, and titlecase, full and simple case folding
    * caseless matching (Unicode Core Specification D144-D147)
    * grapheme, word, and sentence segmentation (UAX #29) and line-break opportunities (UAX #14)
    * terminal width (UAX #11) and truncation to a number of terminal columns

  All functions take and return UTF-8 binaries.

  ## Malformed input

  Transforms such as `normalize/3` and `upcase/2` take a `:malformed` option that says what to do
  with input that is not well-formed UTF-8:

    * `:stop` (the default) - return `{:error, %Mojibake.Error{reason: :malformed_input}}` with
      the byte offset and length of the first ill-formed subsequence
    * `:replace` - replace each maximal ill-formed subsequence with U+FFFD
    * `:skip` - drop each maximal ill-formed subsequence

  Queries such as `graphemes/1`, `grapheme_count/1`, and `terminal_width/2` never fail: they
  treat each maximal ill-formed subsequence as U+FFFD.

  ## Scheduling

  Inputs larger than 16 KiB are processed on a dirty CPU scheduler, so that long strings do not
  block the normal schedulers.
  """

  alias Mojibake.{Error, Nif}

  @typedoc "A normalization form."
  @type form :: :nfc | :nfd | :nfkc | :nfkd | :nfkc_casefold

  @typedoc "What to do with input that is not well-formed UTF-8."
  @type malformed :: :stop | :replace | :skip

  @typedoc """
  A casing locale: `nil` for locale-independent casing, or a BCP 47 language tag such as `"tr"`
  or `:lt`. Only the locales that `SpecialCasing.txt` tailors (Turkish, Azerbaijani, and
  Lithuanian) change the result; any other well-formed tag behaves like `nil`.
  """
  @type locale :: String.t() | atom() | nil

  @typedoc """
  A caseless matching relation from the Unicode Core Specification:

    * `:canonical` - D145, `NFD(toCasefold(NFD(X)))`
    * `:default` - D144, `toCasefold(X)`, without normalization
    * `:compatibility` - D146, compatibility caseless match
    * `:identifier` - D147, `toNFKC_Casefold(NFD(X))`
  """
  @type caseless_mode :: :canonical | :default | :compatibility | :identifier

  @typedoc """
  How East Asian Width Ambiguous characters are measured (UAX #11): one terminal cell
  (`:narrow`) or two, as in CJK terminals (`:east_asian`).
  """
  @type width_profile :: :narrow | :east_asian

  @typedoc "A line-break opportunity and the byte offset it is at."
  @type line_break :: {:mandatory | :allowed, non_neg_integer()}

  @forms [:nfc, :nfd, :nfkc, :nfkd, :nfkc_casefold]
  @quick_check_forms [:nfc, :nfd, :nfkc, :nfkd]
  @policies [:stop, :replace, :skip]
  @caseless_modes [:canonical, :default, :compatibility, :identifier]
  @profiles [:narrow, :east_asian]

  # Normalization

  @doc """
  Normalize `string` to `form` (UAX #15).

  `form` is `:nfc` (the default), `:nfd`, `:nfkc`, `:nfkd`, or `:nfkc_casefold`. When `string` is
  already normalized, it is returned as is, without a copy.

  ## Options

    * `:malformed` - see the module documentation. Defaults to `:stop`.

  ## Examples

      iex> Mojibake.normalize("e\\u0301")
      {:ok, "é"}

      iex> Mojibake.normalize("ﬁ", :nfkc)
      {:ok, "fi"}

      iex> Mojibake.normalize("Straße", :nfkc_casefold)
      {:ok, "strasse"}

      iex> {:error, error} = Mojibake.normalize(<<"a", 0xFF>>)
      iex> {error.reason, error.offset, error.length}
      {:malformed_input, 1, 1}
  """
  @spec normalize(String.t(), form(), keyword()) :: {:ok, String.t()} | {:error, Error.t()}
  def normalize(string, form \\ :nfc, opts \\ [])
      when is_binary(string) and form in @forms and is_list(opts) do
    policy = opts |> Keyword.validate!(malformed: :stop) |> malformed_option()

    string |> Nif.normalize(form, policy) |> result()
  end

  @doc """
  Like `normalize/3`, but returns the string or raises `Mojibake.Error`.

  ## Examples

      iex> Mojibake.normalize!("Å", :nfd)
      "A\\u030A"
  """
  @spec normalize!(String.t(), form(), keyword()) :: String.t()
  def normalize!(string, form \\ :nfc, opts \\ []), do: bang(normalize(string, form, opts))

  @doc """
  Return `true` if `string` is in normalization `form`.

  It runs the quick check of UAX #15 and normalizes only when the quick check cannot decide.
  Input that is not well-formed UTF-8 is never normalized.

  ## Examples

      iex> Mojibake.normalized?("é")
      true

      iex> Mojibake.normalized?("e\\u0301")
      false

      iex> Mojibake.normalized?("e\\u0301", :nfd)
      true
  """
  @spec normalized?(String.t(), form()) :: boolean()
  def normalized?(string, form \\ :nfc) when is_binary(string) and form in @forms do
    quick_check =
      if form in @quick_check_forms do
        Nif.quick_check(string, form)
      else
        {:ok, :maybe}
      end

    case quick_check do
      {:ok, :yes} ->
        true

      {:ok, :no} ->
        false

      {:ok, :maybe} ->
        match?({:ok, ^string}, Nif.normalize(string, form, :stop))

      {:error, :malformed_input} ->
        false

      {:error, :malformed_input, _, _, _} ->
        false

      error ->
        raise Error.from_nif(error)
    end
  end

  @doc """
  The quick check of UAX #15 for `string` and `form`: `:yes`, `:no`, or `:maybe`.

  `form` is `:nfc`, `:nfd`, `:nfkc`, or `:nfkd`. A `:maybe` answer needs a full normalization to
  decide; `normalized?/2` does that.

  ## Examples

      iex> Mojibake.quick_check("abc", :nfc)
      {:ok, :yes}

      iex> Mojibake.quick_check("e\\u0301", :nfc)
      {:ok, :maybe}
  """
  @spec quick_check(String.t(), :nfc | :nfd | :nfkc | :nfkd) ::
          {:ok, :yes | :no | :maybe} | {:error, Error.t()}
  def quick_check(string, form) when is_binary(string) and form in @quick_check_forms do
    string |> Nif.quick_check(form) |> result()
  end

  # Casing

  @doc """
  Convert `string` to uppercase with the full case mappings of Unicode, which can change its
  length.

  ## Options

    * `:locale` - a BCP 47 language tag that tailors the mapping, such as `"tr"`. Defaults to
      `nil`, locale-independent casing. See `t:locale/0`.
    * `:malformed` - see the module documentation. Defaults to `:stop`.

  ## Examples

      iex> Mojibake.upcase("straße")
      {:ok, "STRASSE"}

      iex> Mojibake.upcase("istanbul", locale: "tr")
      {:ok, "İSTANBUL"}
  """
  @spec upcase(String.t(), keyword()) :: {:ok, String.t()} | {:error, Error.t()}
  def upcase(string, opts \\ []) when is_binary(string), do: map_case(string, :upper, opts)

  @doc """
  Like `upcase/2`, but returns the string or raises `Mojibake.Error`.
  """
  @spec upcase!(String.t(), keyword()) :: String.t()
  def upcase!(string, opts \\ []), do: bang(upcase(string, opts))

  @doc """
  Convert `string` to lowercase with the full case mappings of Unicode, including the Greek
  final sigma rule.

  Takes the same options as `upcase/2`.

  ## Examples

      iex> Mojibake.downcase("ΟΔΟΣ")
      {:ok, "οδος"}

      iex> Mojibake.downcase("DİYARBAKIR", locale: "tr")
      {:ok, "diyarbakır"}
  """
  @spec downcase(String.t(), keyword()) :: {:ok, String.t()} | {:error, Error.t()}
  def downcase(string, opts \\ []) when is_binary(string), do: map_case(string, :lower, opts)

  @doc """
  Like `downcase/2`, but returns the string or raises `Mojibake.Error`.
  """
  @spec downcase!(String.t(), keyword()) :: String.t()
  def downcase!(string, opts \\ []), do: bang(downcase(string, opts))

  @doc """
  Titlecase each word of `string`: the first cased character of each UAX #29 word is
  titlecased and the rest are lowercased.

  Takes the same options as `upcase/2`.

  ## Examples

      iex> Mojibake.titlecase("hello WORLD")
      {:ok, "Hello World"}

      iex> Mojibake.titlecase("ǆungla")
      {:ok, "ǅungla"}
  """
  @spec titlecase(String.t(), keyword()) :: {:ok, String.t()} | {:error, Error.t()}
  def titlecase(string, opts \\ []) when is_binary(string), do: map_case(string, :title, opts)

  @doc """
  Like `titlecase/2`, but returns the string or raises `Mojibake.Error`.
  """
  @spec titlecase!(String.t(), keyword()) :: String.t()
  def titlecase!(string, opts \\ []), do: bang(titlecase(string, opts))

  @doc """
  Case-fold `string` for caseless comparison.

  ## Options

    * `:mode` - `:full` (the default) applies the `C` and `F` mappings of `CaseFolding.txt`, which
      can make the string longer. `:simple` applies the `C` and `S` mappings, which map each
      character to one character.
    * `:malformed` - see the module documentation. Defaults to `:stop`.

  To compare two strings without regard to case, `caseless_equal?/3` is usually what you want.

  ## Examples

      iex> Mojibake.casefold("Straße")
      {:ok, "strasse"}

      iex> Mojibake.casefold("Straße", mode: :simple)
      {:ok, "straße"}
  """
  @spec casefold(String.t(), keyword()) :: {:ok, String.t()} | {:error, Error.t()}
  def casefold(string, opts \\ []) when is_binary(string) do
    {mode, opts} = Keyword.pop(opts, :mode, :full)

    type =
      case mode do
        :full ->
          :fold

        :simple ->
          :fold_simple

        other ->
          raise ArgumentError, "expected :mode to be :full or :simple, got: #{inspect(other)}"
      end

    if Keyword.has_key?(opts, :locale) do
      raise ArgumentError, "casefold/2 does not take a :locale option"
    end

    map_case(string, type, opts)
  end

  @doc """
  Like `casefold/2`, but returns the string or raises `Mojibake.Error`.
  """
  @spec casefold!(String.t(), keyword()) :: String.t()
  def casefold!(string, opts \\ []), do: bang(casefold(string, opts))

  @doc """
  Return `true` if `first` and `second` are equal without regard to case, under the caseless
  matching relation `mode` (see `t:caseless_mode/0`). Defaults to `:canonical`.

  Ill-formed UTF-8 is compared as if each maximal ill-formed subsequence were U+FFFD.

  ## Examples

      iex> Mojibake.caseless_equal?("Straße", "STRASSE")
      true

      iex> Mojibake.caseless_equal?("Å", "a\\u030A")
      true

      iex> Mojibake.caseless_equal?("Å", "a\\u030A", :default)
      false

      iex> Mojibake.caseless_equal?("ﬁ", "FI", :compatibility)
      true
  """
  @spec caseless_equal?(String.t(), String.t(), caseless_mode()) :: boolean()
  def caseless_equal?(first, second, mode \\ :canonical)
      when is_binary(first) and is_binary(second) and mode in @caseless_modes do
    first |> Nif.caseless_match(second, mode, :replace) |> value!()
  end

  # Segmentation

  @doc """
  Split `string` into extended grapheme clusters (UAX #29).

  Each grapheme is a sub-binary of `string`; nothing is copied.

  ## Examples

      iex> Mojibake.graphemes("e\\u0301🇮🇹👨‍👩‍👧")
      ["e\\u0301", "🇮🇹", "👨‍👩‍👧"]
  """
  @spec graphemes(String.t()) :: [String.t()]
  def graphemes(string) when is_binary(string), do: Nif.segments(string, :grapheme)

  @doc """
  Split `string` at the word boundaries of UAX #29.

  The result includes the spaces and punctuation between words, so the segments always join
  back into `string`.

  ## Examples

      iex> Mojibake.words("Hello, world!")
      ["Hello", ",", " ", "world", "!"]

      iex> Mojibake.words("can't stop 3.14")
      ["can't", " ", "stop", " ", "3.14"]
  """
  @spec words(String.t()) :: [String.t()]
  def words(string) when is_binary(string), do: Nif.segments(string, :word)

  @doc """
  Split `string` into sentences (UAX #29). Each sentence keeps its trailing spaces.

  ## Examples

      iex> Mojibake.sentences("Hello. How are you? Fine!")
      ["Hello. ", "How are you? ", "Fine!"]
  """
  @spec sentences(String.t()) :: [String.t()]
  def sentences(string) when is_binary(string), do: Nif.segments(string, :sentence)

  @doc """
  The line-break opportunities of `string` (UAX #14), as `{kind, byte_offset}` tuples in order.

  `kind` is `:mandatory` where a line must break, for example after a newline, and `:allowed`
  where it may. A line can break before the byte at `byte_offset`. The end of the text is always
  the last opportunity.

  ## Examples

      iex> Mojibake.line_breaks("Hello world")
      [allowed: 6, mandatory: 11]

      iex> Mojibake.line_breaks("a\\nb")
      [mandatory: 2, mandatory: 3]
  """
  @spec line_breaks(String.t()) :: [line_break()]
  def line_breaks(string) when is_binary(string), do: Nif.line_breaks(string)

  @doc """
  The number of extended grapheme clusters in `string`.

  ## Examples

      iex> Mojibake.grapheme_count("e\\u0301🇮🇹")
      2
  """
  @spec grapheme_count(String.t()) :: non_neg_integer()
  def grapheme_count(string) when is_binary(string), do: count(string, :grapheme)

  @doc """
  The number of words in `string`: the word-break segments that contain a letter or a digit.

  Scripts that are usually segmented with a dictionary, such as Chinese, Japanese, and Thai,
  count about one word per character.

  ## Examples

      iex> Mojibake.word_count("Hello, world! 42")
      3
  """
  @spec word_count(String.t()) :: non_neg_integer()
  def word_count(string) when is_binary(string), do: count(string, :word)

  @doc """
  The number of sentences in `string`.

  ## Examples

      iex> Mojibake.sentence_count("One. Two? Three!")
      3
  """
  @spec sentence_count(String.t()) :: non_neg_integer()
  def sentence_count(string) when is_binary(string), do: count(string, :sentence)

  @doc """
  The number of codepoints in `string`. Each maximal ill-formed subsequence counts as one.

  ## Examples

      iex> Mojibake.codepoint_count("e\\u0301")
      2
  """
  @spec codepoint_count(String.t()) :: non_neg_integer()
  def codepoint_count(string) when is_binary(string), do: count(string, :codepoint)

  @doc """
  The first `count` extended grapheme clusters of `string`, or all of `string` if it has fewer.

  ## Examples

      iex> Mojibake.grapheme_prefix("e\\u0301🇮🇹abc", 2)
      "e\\u0301🇮🇹"
  """
  @spec grapheme_prefix(String.t(), non_neg_integer()) :: String.t()
  def grapheme_prefix(string, count)
      when is_binary(string) and is_integer(count) and count >= 0 do
    binary_part(string, 0, Nif.prefix_length(string, :grapheme, clamp(count)))
  end

  @doc """
  The first `count` segments that `words/1` returns, including spaces and punctuation, or all of
  `string` if it has fewer.

  ## Examples

      iex> Mojibake.word_prefix("Hello, world!", 4)
      "Hello, world"
  """
  @spec word_prefix(String.t(), non_neg_integer()) :: String.t()
  def word_prefix(string, count) when is_binary(string) and is_integer(count) and count >= 0 do
    binary_part(string, 0, Nif.prefix_length(string, :word, clamp(count)))
  end

  # Terminal width

  @doc """
  The number of terminal columns that `string` takes, for printable single-line text.

  Wide and fullwidth characters (UAX #11) and most emoji take two columns, combining marks and
  other zero-width characters take none.

  ## Options

    * `:profile` - `:narrow` (the default) or `:east_asian`. See `t:width_profile/0`.

  ## Examples

      iex> Mojibake.terminal_width("abc")
      3

      iex> Mojibake.terminal_width("日本語")
      6

      iex> Mojibake.terminal_width("e\\u0301")
      1

      iex> Mojibake.terminal_width("α", profile: :east_asian)
      2
  """
  @spec terminal_width(String.t(), keyword()) :: non_neg_integer()
  def terminal_width(string, opts \\ []) when is_binary(string) and is_list(opts) do
    opts = Keyword.validate!(opts, profile: :narrow)

    string |> Nif.terminal_width(profile_option(opts), :replace) |> value!()
  end

  @doc """
  The longest prefix of `string` that fits in `columns` terminal columns, without splitting a
  grapheme.

  ## Options

    * `:profile` - `:narrow` (the default) or `:east_asian`. See `t:width_profile/0`.
    * `:break` - `:grapheme` (the default) cuts between graphemes. `:word` cuts only between the
      segments that `words/1` returns, so it does not split words.

  ## Examples

      iex> Mojibake.truncate_to_width("日本語", 5)
      "日本"

      iex> Mojibake.truncate_to_width("Hello world", 8)
      "Hello wo"

      iex> Mojibake.truncate_to_width("Hello world", 8, break: :word)
      "Hello "
  """
  @spec truncate_to_width(String.t(), non_neg_integer(), keyword()) :: String.t()
  def truncate_to_width(string, columns, opts \\ [])
      when is_binary(string) and is_integer(columns) and columns >= 0 and is_list(opts) do
    opts = Keyword.validate!(opts, profile: :narrow, break: :grapheme)

    kind =
      case Keyword.fetch!(opts, :break) do
        kind when kind in [:grapheme, :word] ->
          kind

        other ->
          raise ArgumentError, "expected :break to be :grapheme or :word, got: #{inspect(other)}"
      end

    length = Nif.prefix_length_for_width(string, kind, profile_option(opts), clamp(columns))

    binary_part(string, 0, length)
  end

  # Library information

  @doc """
  The version of the Mojibake C library.

  ## Examples

      iex> Mojibake.version()
      "0.8.0"
  """
  @spec version() :: String.t()
  def version, do: Nif.version()

  @doc """
  The version of Unicode that the data in this library follows.

  ## Examples

      iex> Mojibake.unicode_version()
      "18.0.0"
  """
  @spec unicode_version() :: String.t()
  def unicode_version, do: Nif.unicode_version()

  @doc """
  The optional features the C library was compiled with: any of `:names`, `:collation`, `:idna`,
  and `:security`.

  They are all on by default. See the README to turn them off.
  """
  @spec features() :: [:names | :collation | :idna | :security]
  def features, do: Nif.features()

  # Helpers

  defp map_case(string, type, opts) do
    opts = Keyword.validate!(opts, [:locale, malformed: :stop])
    policy = malformed_option(opts)

    locale =
      case Keyword.get(opts, :locale) do
        nil ->
          nil

        tag when is_binary(tag) ->
          tag

        tag when is_atom(tag) ->
          tag |> Atom.to_string() |> String.replace("_", "-")

        other ->
          raise ArgumentError,
                "expected :locale to be a string, an atom, or nil, got: #{inspect(other)}"
      end

    string |> Nif.map_case(type, locale, policy) |> result()
  end

  defp malformed_option(opts) do
    case Keyword.get(opts, :malformed, :stop) do
      policy when policy in @policies ->
        policy

      other ->
        raise ArgumentError,
              "expected :malformed to be :stop, :replace, or :skip, got: #{inspect(other)}"
    end
  end

  defp profile_option(opts) do
    case Keyword.fetch!(opts, :profile) do
      profile when profile in @profiles ->
        profile

      other ->
        raise ArgumentError,
              "expected :profile to be :narrow or :east_asian, got: #{inspect(other)}"
    end
  end

  defp count(string, kind), do: string |> Nif.count(kind, :replace) |> value!()

  # The NIF takes a 64-bit count; anything larger means "all of it".
  @max_count 0xFFFFFFFFFFFFFFFF
  defp clamp(count), do: min(count, @max_count)

  defp result({:ok, value}), do: {:ok, value}
  defp result(error), do: {:error, Error.from_nif(error)}

  defp value!({:ok, value}), do: value
  defp value!(error), do: raise(Error.from_nif(error))

  defp bang({:ok, value}), do: value
  defp bang({:error, error}), do: raise(error)
end
