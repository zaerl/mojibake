import os
import sys
import re

# A dictionary of exported functions usage
type FuncCoverage = dict[str, int]
type Coverage = dict[str, FuncCoverage]

coverage: Coverage = {}


def comparator(a: str, b: str) -> int:
    if (coverage[a]["u"] and coverage[b]["u"] or
            not coverage[a]["c"] and not coverage[b]["u"]):
        if a < b:
            return -1
        elif a > b:
            return 1
        else:
            return 0

    if coverage[a]["u"] and not coverage[b]["u"]:
        return -1

    if not coverage[a]["u"] and coverage[b]["u"]:
        return 1


def scan_file(filepath: str):
    try:
        with open(filepath, 'r') as f:
            prog = re.compile(r'MJB_EXPORT.+[ \*]([a-z_]+)\([^)]*\)\s*{$')
            for line in f:
                result = prog.match(line.strip())
                if result:
                    coverage[result.group(1)] = {"u": 0, "c": 0}
    except IOError as e:
        print(f"Error reading file {filepath}: {e}", file=sys.stderr)


def find_exports(directory: str):
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(('.c', '.h')):
                filepath = os.path.join(root, file)
                scan_file(filepath)


def scan_test_file(filepath: str):
    try:
        with open(filepath, 'r') as f:
            for line in f:
                line = line.strip()
                previous_result = ""
                result = re.match(r"^ATT_ASSERT\(([a-z_]+)\(.+$", line)
                if result:
                    key = result.group(1).strip()
                    if key in coverage:
                        coverage[key]["u"] += 1
                        previous_result = key
                    elif previous_result:
                        if previous_result in coverage:
                            coverage[previous_result]["u"] += 1
    except IOError as e:
        print(f"Error reading test file {filepath}: {e}", file=sys.stderr)


def find_tests(directory: str):
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(('.c')):
                filepath = os.path.join(root, file)
                scan_test_file(filepath)


def print_coverage():
    total = 0
    max_name_length = 0

    for key, value in coverage.items():
        total += 1
        max_name_length = max(max_name_length, len(key))
    sorted_cov = sorted(
        coverage.items(),
        key=lambda item: (-item[1]["u"], item[0])
    )
    post = " " * (max_name_length - 5)
    bar = "-" * max_name_length

    print("# Test coverage\n")
    print(f"| Test {post} | Coverage |")
    print(f"| {bar} | -------- |")

    for key, value in sorted_cov:
        post = " " * (max_name_length - len(key))
        print(f"| {key}{post} | {value["u"]}", end="")
        post = " " * (8 - len(str(value["u"])))
        print(f"{post} |")

    post = " " * (max_name_length - 10)
    print(f"| **Total** {post} | **{total}**", end="")

    post = " " * (4 - len(str(total)))
    print(f"{post} |")


def main():
    source_dir = "./src" if len(sys.argv) < 2 else sys.argv[1]

    if not os.path.isdir(source_dir):
        print(f"Error: {source_dir} is not a valid directory", file=sys.stderr)
        sys.exit(1)

    test_dir = "./tests" if len(sys.argv) < 3 else sys.argv[2]

    if not os.path.isdir(test_dir):
        print(f"Error: {test_dir} is not a valid directory", file=sys.stderr)
        sys.exit(1)

    find_exports(source_dir)
    find_tests(test_dir)
    print_coverage()


if __name__ == "__main__":
    main()
