# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Error do
  @moduledoc """
  An error reported by Mojibake.

  Functions that can fail return `{:error, %Mojibake.Error{}}`; their `!` variants raise it.

  `reason` is one of:

    * `:malformed_input` - the input is not well-formed UTF-8 and the `:malformed` option is
      `:stop`. `offset` and `length` give the byte range of the first ill-formed subsequence and
      `detail` says what is wrong with it, for example `:missing_continuation`.
    * `:invalid_locale` - a BCP 47 language tag, such as the `:locale` option, is not
      well-formed.
    * `:invalid_domain` - `Mojibake.IDNA` found the domain name invalid. `detail` is the list of
      `t:Mojibake.IDNA.error/0`.
    * `:no_memory` - the C library could not allocate memory.
    * `:overflow` - a size computation overflowed.
    * any other `mjb_status` of the C library, as an atom: `:invalid_argument`,
      `:invalid_encoding`, `:invalid_codepoint`, `:invalid_form`, `:unsupported`,
      `:output_too_small`, `:callback_stopped`, `:not_found`, `:feature_not_enabled`, or
      `:end_of_input`.
  """

  @type reason :: atom()

  @type t :: %__MODULE__{
          reason: reason(),
          offset: non_neg_integer() | nil,
          length: non_neg_integer() | nil,
          detail: atom() | [atom()] | nil
        }

  defexception [:reason, :offset, :length, :detail]

  @impl true
  def message(%__MODULE__{reason: :malformed_input, offset: offset, length: length, detail: detail})
      when is_integer(offset) do
    "malformed input: #{describe(detail)} at byte #{offset} (#{length} byte#{if length == 1, do: "", else: "s"})"
  end

  def message(%__MODULE__{reason: :invalid_locale}), do: "invalid BCP 47 language tag"

  def message(%__MODULE__{reason: :invalid_domain, detail: errors}) when is_list(errors),
    do: "invalid domain name: " <> Enum.map_join(errors, ", ", &describe/1)

  def message(%__MODULE__{reason: reason}),
    do: reason |> Atom.to_string() |> String.replace("_", " ")

  defp describe(nil), do: "ill-formed sequence"
  defp describe(detail), do: detail |> Atom.to_string() |> String.replace("_", " ")

  @doc false
  # Convert a raw NIF error into an exception.
  def from_nif({:error, :malformed_input, offset, length, detail}),
    do: %__MODULE__{reason: :malformed_input, offset: offset, length: length, detail: detail}

  def from_nif({:error, reason}) when is_atom(reason), do: %__MODULE__{reason: reason}
end
