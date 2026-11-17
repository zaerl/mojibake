# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Bidi do
  @moduledoc """
  The Unicode Bidirectional Algorithm (UAX #9), which lays out text that mixes left-to-right
  scripts, such as Latin, with right-to-left ones, such as Hebrew and Arabic.

  Text is stored in logical order, the order it is typed and read in. `resolve/2` computes the
  embedding level of every character of a paragraph: even levels are left to right, odd levels
  right to left. A line of the paragraph is then reordered for display with `reorder/2`, split
  into directional runs with `runs/2`, or rendered in visual order with `visual/2`:

      iex> paragraph = Mojibake.Bidi.resolve!("abc אבג")
      iex> paragraph.direction
      :ltr
      iex> Mojibake.Bidi.levels(paragraph)
      [0, 0, 0, 0, 1, 1, 1]
      iex> Mojibake.Bidi.visual(paragraph)
      "abc גבא"

  ## Positions and lines

  Positions are indices into the characters of the paragraph, `0` to `paragraph.length - 1`,
  not codepoint indices of the text: rule X9 removes the embedding and override controls and the
  Boundary Neutrals, such as U+200D ZERO WIDTH JOINER, from the paragraph. `chars/1` gives the
  byte offset of each character in the text.

  The functions that take a `line` take a range of positions with step 1, such as `0..9`, and
  default to the whole paragraph. Break a paragraph into lines yourself, with
  `Mojibake.line_breaks/1` and the width of your output, and lay out each line separately.

  A paragraph is one paragraph: split text at paragraph separators, such as `"\\n"`, first.
  Resolving it is the expensive step; reordering a line of it is cheap, and the resolved
  paragraph can be shared between processes.
  """

  alias Mojibake.{Char, Encoding, Error, Nif}

  defmodule Paragraph do
    @moduledoc """
    A paragraph resolved by `Mojibake.Bidi.resolve/2`.

      * `text` - the text of the paragraph
      * `direction` - the paragraph direction, `:ltr` or `:rtl`
      * `level` - the paragraph embedding level, `0` for left to right and `1` for right to left
      * `length` - the number of characters of the paragraph, after rule X9 removed the controls

    The resolved levels are held by the NIF and freed when the paragraph is garbage collected.
    """

    @derive {Inspect, except: [:ref]}
    @enforce_keys [:text, :direction, :level, :length, :ref]
    defstruct [:text, :direction, :level, :length, :ref]

    @type t :: %__MODULE__{
            text: String.t(),
            direction: :ltr | :rtl,
            level: 0 | 1,
            length: non_neg_integer(),
            ref: reference()
          }
  end

  @typedoc """
  The base direction of a paragraph. `:auto` takes it from the first strong character (rules P2
  and P3), and is left to right if there is none.
  """
  @type direction :: :ltr | :rtl | :auto

  @typedoc """
  A character of a resolved paragraph:

    * `codepoint` - the character
    * `offset` - its byte offset in the paragraph text
    * `level` - its resolved embedding level; odd levels are right to left
    * `class` - its class after the weak and neutral rules (W1-W7, N1-N2) resolved it, an atom
      of `t:Mojibake.Char.bidi_class/0`. `Mojibake.Char.bidi_class/1` gives its original class.
    * `mirror` - the Bidi_Mirroring_Glyph to display instead of the character (rule L4), when it
      is mirrored and its level is odd, else `nil`
  """
  @type char_info :: %{
          codepoint: non_neg_integer(),
          offset: non_neg_integer(),
          level: non_neg_integer(),
          class: Char.bidi_class(),
          mirror: non_neg_integer() | nil
        }

  @typedoc """
  A run of characters with the same embedding level on a reordered line:

    * `visual` - the positions of the run in the visual order, indices into the list that
      `reorder/2` returns for the same line
    * `logical` - the paragraph positions the run covers, in logical order
    * `level` - the embedding level of the run
    * `direction` - `:rtl` for odd levels, `:ltr` otherwise
  """
  @type run :: %{
          visual: Range.t(),
          logical: Range.t(),
          level: non_neg_integer(),
          direction: :ltr | :rtl
        }

  @directions [:ltr, :rtl, :auto]

  @doc """
  Resolve the embedding levels of `text`, one paragraph, with base direction `direction`.

  Returns `{:error, %Mojibake.Error{}}` if `text` is not well-formed UTF-8.

  ## Examples

      iex> {:ok, paragraph} = Mojibake.Bidi.resolve("שלום world")
      iex> {paragraph.direction, paragraph.level, paragraph.length}
      {:rtl, 1, 10}

      iex> {:ok, paragraph} = Mojibake.Bidi.resolve("שלום world", :ltr)
      iex> paragraph.direction
      :ltr

      iex> Mojibake.Bidi.resolve(<<?a, 0xFF>>)
      {:error, %Mojibake.Error{reason: :malformed_input, offset: 1, length: 1, detail: :invalid_leading_byte}}
  """
  @spec resolve(String.t(), direction()) :: {:ok, Paragraph.t()} | {:error, Error.t()}
  def resolve(text, direction \\ :auto) when is_binary(text) and direction in @directions do
    case Nif.bidi_resolve(text, direction) do
      {:ok, {ref, level, length}} ->
        {:ok,
         %Paragraph{
           text: text,
           direction: level_direction(level),
           level: level,
           length: length,
           ref: ref
         }}

      {:error, :malformed_input} = error ->
        # The library does not report where; the UTF-8 validator does.
        case Encoding.validate(text, :utf8) do
          {:error, _} = detailed -> detailed
          :ok -> {:error, Error.from_nif(error)}
        end

      error ->
        {:error, Error.from_nif(error)}
    end
  end

  @doc """
  Like `resolve/2`, but raises `Mojibake.Error`.

  ## Examples

      iex> Mojibake.Bidi.resolve!("مرحبا").direction
      :rtl
  """
  @spec resolve!(String.t(), direction()) :: Paragraph.t()
  def resolve!(text, direction \\ :auto) do
    case resolve(text, direction) do
      {:ok, paragraph} -> paragraph
      {:error, error} -> raise error
    end
  end

  @doc """
  Return the paragraph direction of `text`: the direction of its first strong character, or
  `:ltr` if it has none (rules P2 and P3). Raises `Mojibake.Error` if `text` is not well-formed
  UTF-8.

  ## Examples

      iex> Mojibake.Bidi.direction("Hello")
      :ltr

      iex> Mojibake.Bidi.direction("123 שלום")
      :rtl

      iex> Mojibake.Bidi.direction("123")
      :ltr
  """
  @spec direction(String.t()) :: :ltr | :rtl
  def direction(text), do: resolve!(text).direction

  @doc """
  Return the characters of `paragraph`, in logical order. See `t:char_info/0`.

  ## Examples

      iex> "(א)" |> Mojibake.Bidi.resolve!() |> Mojibake.Bidi.chars() |> hd()
      %{codepoint: ?(, offset: 0, level: 1, class: :right_to_left, mirror: ?)}
  """
  @spec chars(Paragraph.t()) :: [char_info()]
  def chars(%Paragraph{ref: ref}) do
    for {codepoint, offset, level, class, mirror} <- Nif.bidi_chars(ref) do
      %{
        codepoint: codepoint,
        offset: offset,
        level: level,
        class: Char.bidi_class_atom(class),
        mirror: if(mirror == 0, do: nil, else: mirror)
      }
    end
  end

  @doc """
  Return the resolved embedding level of each character of `paragraph`, in logical order.

  ## Examples

      iex> "ab אב 12" |> Mojibake.Bidi.resolve!() |> Mojibake.Bidi.levels()
      [0, 0, 0, 1, 1, 1, 2, 2]
  """
  @spec levels(Paragraph.t()) :: [non_neg_integer()]
  def levels(%Paragraph{ref: ref}) do
    for {_, _, level, _, _} <- Nif.bidi_chars(ref), do: level
  end

  @doc """
  Reorder a line of `paragraph` for display (rules L1 and L2). Returns the positions of the line
  in visual order, from left to right.

  Raises `ArgumentError` if `line` is not a range of positions of the paragraph.

  ## Examples

      iex> paragraph = Mojibake.Bidi.resolve!("abc אבג")
      iex> Mojibake.Bidi.reorder(paragraph)
      [0, 1, 2, 3, 6, 5, 4]
      iex> Mojibake.Bidi.reorder(paragraph, 4..6)
      [6, 5, 4]
  """
  @spec reorder(Paragraph.t(), Range.t() | nil) :: [non_neg_integer()]
  def reorder(%Paragraph{} = paragraph, line \\ nil) do
    {start, stop} = line!(paragraph, line)

    paragraph.ref |> Nif.bidi_reorder(start, stop) |> value!()
  end

  @doc """
  Split a reordered line of `paragraph` into runs of the same embedding level, from left to
  right. See `t:run/0`.

  Raises `ArgumentError` if `line` is not a range of positions of the paragraph.

  ## Examples

      iex> "abc אבג" |> Mojibake.Bidi.resolve!() |> Mojibake.Bidi.runs()
      [
        %{visual: 0..3, logical: 0..3, level: 0, direction: :ltr},
        %{visual: 4..6, logical: 4..6, level: 1, direction: :rtl}
      ]
  """
  @spec runs(Paragraph.t(), Range.t() | nil) :: [run()]
  def runs(%Paragraph{} = paragraph, line \\ nil) do
    {start, stop} = line!(paragraph, line)

    for {visual_start, visual_end, logical_start, logical_end, level} <-
          paragraph.ref |> Nif.bidi_runs(start, stop) |> value!() do
      %{
        visual: visual_start..(visual_end - 1)//1,
        logical: logical_start..(logical_end - 1)//1,
        level: level,
        direction: level_direction(level)
      }
    end
  end

  @doc """
  Return a line in visual order, with mirrored characters, such as parentheses in right-to-left
  text, replaced by their mirroring glyphs (rule L4). The controls that rule X9 removes are left
  out.

  Takes a resolved paragraph and an optional `line`, or a text and an optional base direction.
  Visual order is for display on a device that does not do bidirectional layout itself, such as
  a terminal; do not store or process text in it.

  Raises `ArgumentError` if `line` is not a range of positions of the paragraph, and
  `Mojibake.Error` if `text` is not well-formed UTF-8.

  ## Examples

      iex> Mojibake.Bidi.visual("abc אבג")
      "abc גבא"

      iex> Mojibake.Bidi.visual("abc (אבג)", :rtl)
      "(גבא) abc"

      iex> paragraph = Mojibake.Bidi.resolve!("abc אבג")
      iex> Mojibake.Bidi.visual(paragraph, 2..5)
      "c בא"
  """
  @spec visual(Paragraph.t(), Range.t() | nil) :: String.t()
  @spec visual(String.t(), direction()) :: String.t()
  def visual(paragraph_or_text, line_or_direction \\ nil)

  def visual(%Paragraph{} = paragraph, line) do
    {start, stop} = line!(paragraph, line)

    paragraph.ref |> Nif.bidi_visual(start, stop) |> value!()
  end

  def visual(text, direction) when is_binary(text) do
    paragraph = resolve!(text, direction || :auto)

    visual(paragraph, nil)
  end

  defp level_direction(level) when rem(level, 2) == 1, do: :rtl
  defp level_direction(_level), do: :ltr

  # A line as half-open positions {start, stop}.
  defp line!(%Paragraph{length: length}, nil), do: {0, length}

  defp line!(%Paragraph{length: length}, first..last//1 = line) do
    if first >= 0 and last < length and first <= last + 1 do
      {first, last + 1}
    else
      raise ArgumentError,
            "expected a line of positions within 0..#{length - 1}//1, got: #{inspect(line)}"
    end
  end

  defp line!(_paragraph, line) do
    raise ArgumentError, "expected a line as a range with step 1, got: #{inspect(line)}"
  end

  defp value!({:ok, value}), do: value
  defp value!(error), do: raise(Error.from_nif(error))
end
