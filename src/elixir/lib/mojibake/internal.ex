# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

defmodule Mojibake.Internal do
  @moduledoc false

  # Helpers shared by the modules that wrap the NIFs.

  alias Mojibake.Error

  @policies [:stop, :replace, :skip]

  # The :malformed option, validated.
  def malformed_option(opts, default) do
    case Keyword.get(opts, :malformed, default) do
      policy when policy in @policies ->
        policy

      other ->
        raise ArgumentError,
              "expected :malformed to be :stop, :replace, or :skip, got: #{inspect(other)}"
    end
  end

  # An option that must be one of `allowed`.
  def option(opts, key, default, allowed) do
    value = Keyword.get(opts, key, default)

    if value in allowed do
      value
    else
      raise ArgumentError,
            "expected #{inspect(key)} to be one of #{inspect(allowed)}, got: #{inspect(value)}"
    end
  end

  # {:ok, value} or {:error, %Mojibake.Error{}} from a NIF result.
  def result({:ok, value}), do: {:ok, value}
  def result(error), do: {:error, Error.from_nif(error)}

  # The value of a NIF result, or raise its error.
  def value!({:ok, value}), do: value
  def value!(error), do: raise(Error.from_nif(error))

  # The value of a public {:ok, value} | {:error, error} result, or raise the error.
  def bang({:ok, value}), do: value
  def bang({:error, error}), do: raise(error)
end
