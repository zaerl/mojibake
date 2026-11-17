# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

# The conformance tests read the Unicode test files that
# utils/generate/scripts/download-test-data.sh downloads into the repository's unicode-data/
# directory. Set MOJIBAKE_TEST_DATA to read them from somewhere else.
data_dir = System.get_env("MOJIBAKE_TEST_DATA") || Path.expand("../../../unicode-data", __DIR__)
Application.put_env(:mojibake, :test_data_dir, data_dir)

exclude =
  if File.regular?(Path.join(data_dir, "UCD/NormalizationTest.txt")) do
    []
  else
    IO.puts("Skipping the conformance tests: no Unicode test data in #{data_dir}")
    [:conformance]
  end

ExUnit.start(exclude: exclude)
