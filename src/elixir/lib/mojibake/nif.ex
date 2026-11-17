# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Nif do
  @moduledoc false

  # Raw bindings to c_src/mojibake_nif.c. The public modules validate arguments and shape the
  # results; call them instead.

  @on_load :load_nif

  def load_nif do
    path = :filename.join(:code.priv_dir(:mojibake), ~c"mojibake_nif")
    :erlang.load_nif(path, 0)
  end

  def normalize(_text, _form, _policy), do: :erlang.nif_error(:not_loaded)
  def quick_check(_text, _form), do: :erlang.nif_error(:not_loaded)
  def map_case(_text, _type, _locale, _policy), do: :erlang.nif_error(:not_loaded)
  def caseless_match(_first, _second, _mode, _policy), do: :erlang.nif_error(:not_loaded)
  def segments(_text, _kind), do: :erlang.nif_error(:not_loaded)
  def line_breaks(_text), do: :erlang.nif_error(:not_loaded)
  def count(_text, _kind, _policy), do: :erlang.nif_error(:not_loaded)
  def prefix_length(_text, _kind, _count), do: :erlang.nif_error(:not_loaded)

  def prefix_length_for_width(_text, _kind, _profile, _columns),
    do: :erlang.nif_error(:not_loaded)

  def terminal_width(_text, _profile, _policy), do: :erlang.nif_error(:not_loaded)
  def version, do: :erlang.nif_error(:not_loaded)
  def unicode_version, do: :erlang.nif_error(:not_loaded)
  def features, do: :erlang.nif_error(:not_loaded)
end
