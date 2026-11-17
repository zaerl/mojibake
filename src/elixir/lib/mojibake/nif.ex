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
  def codepoint_info(_codepoint), do: :erlang.nif_error(:not_loaded)
  def numeric_value(_codepoint), do: :erlang.nif_error(:not_loaded)
  def property(_codepoint, _property, _binary), do: :erlang.nif_error(:not_loaded)
  def block(_codepoint), do: :erlang.nif_error(:not_loaded)
  def script(_codepoint), do: :erlang.nif_error(:not_loaded)
  def script_extensions(_codepoint), do: :erlang.nif_error(:not_loaded)
  def emoji_properties(_codepoint), do: :erlang.nif_error(:not_loaded)
  def codepoint_is(_codepoint, _kind), do: :erlang.nif_error(:not_loaded)
  def plane_name(_plane, _abbreviation), do: :erlang.nif_error(:not_loaded)
  def emoji_sequence(_text), do: :erlang.nif_error(:not_loaded)
  def hangul_syllable_name(_codepoint), do: :erlang.nif_error(:not_loaded)
  def hangul_decompose(_syllable), do: :erlang.nif_error(:not_loaded)
  def hangul_compose(_leading, _vowel, _trailing), do: :erlang.nif_error(:not_loaded)

  def collation_compare(_first, _second, _variable, _strength, _policy),
    do: :erlang.nif_error(:not_loaded)

  def collation_key(_text, _variable, _strength, _policy), do: :erlang.nif_error(:not_loaded)
  def idna(_domain, _form), do: :erlang.nif_error(:not_loaded)
  def confusable_skeleton(_text), do: :erlang.nif_error(:not_loaded)
  def confusable_match(_first, _second, _policy), do: :erlang.nif_error(:not_loaded)
  def resolved_script_set(_text), do: :erlang.nif_error(:not_loaded)
  def is_identifier(_text, _profile), do: :erlang.nif_error(:not_loaded)
  def detect_encoding(_binary), do: :erlang.nif_error(:not_loaded)
  def validate_encoding(_binary, _encoding), do: :erlang.nif_error(:not_loaded)
  def convert_encoding(_binary, _from, _to, _policy), do: :erlang.nif_error(:not_loaded)
  def locale_parse(_tag), do: :erlang.nif_error(:not_loaded)
  def limits, do: :erlang.nif_error(:not_loaded)
end
