import argparse
import groov


def select_name_func(choice):
    def to_lower(s):
        return s.lower()

    def to_upper(s):
        return s.upper()

    def keep(s):
        return s

    return dict(keep=keep, lower=to_lower, upper=to_upper)[choice]


# def generate_groups(groups, ):
def generate_groups(groups, config):
    namespace = config.namespace
    name_func = select_name_func(config.naming)
    bus_type = config.bus
    includes = config.includes

    def indent(lines, len=4):
        if lines:
            prefix = "\n" + (" " * len)
            infix = "," + prefix
            return infix + infix.join(lines)
        else:
            return ""

    def to_integral_type(bit_width):
        if bit_width == 1:
            return "bool"

        for i in [8, 16, 32, 64]:
            if bit_width <= i:
                return f"std::uint_fast{i}_t"

    def generate_field(f):
        integral_type = to_integral_type(f.msb - f.lsb + 1)
        return f"""groov::field<"{name_func(f.name)}", {integral_type}, {f.msb}u, {f.lsb}u, {f.access}>"""

    def generate_register(r):
        fields = indent([generate_field(f) for f in r.fields], len=12)
        return f"""groov::reg<"{name_func(r.name)}", std::uint32_t, {hex(r.address)}u{fields}>"""

    def generate_group(g):
        registers = indent([generate_register(r) for r in g.registers], len=8)
        return f"""constexpr auto {name_func(g.name)} = \n    groov::group<"{name_func(g.name)}", {bus_type}{registers}>{{}};\n"""

    with open(f"{config.output}", "w") as f:
        print("#pragma once", file=f)
        for include in includes:
            print(f"#include <{include}>", file=f)

        print("#include <groov/mmio_bus.hpp>", file=f)
        print("#include <groov/config.hpp>", file=f)
        print("#include <cstdint>", file=f)
        print("", file=f)

        g = groups[config.group]
        print(f"namespace {namespace} {{", file=f)
        print(generate_group(g), end="", file=f)
        print(f"}} // namespace {namespace}", file=f)


def generate(config):
    for f in config.parser_modules:
        exec(open(f).read())
    parse_fn = eval(config.parse_fn)
    groups = parse_fn(config.input)
    generate_groups(groups, config)


# Below this line is just for use as a script


def parse_cmdline():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--bus",
        type=str,
        default="groov::mmio_bus<>",
        help="Specify a custom bus interface for register groups to use.",
    )
    parser.add_argument(
        "--group",
        type=str,
        required=True,
        help="Name of the register group to generate.",
    )
    parser.add_argument(
        "--input",
        type=str,
        required=True,
        help=("Full path to file with register definitions."),
    )
    parser.add_argument(
        "--includes",
        type=str,
        nargs="*",
        default=[],
        help="One or more include files to add.",
    )
    parser.add_argument(
        "--namespace",
        type=str,
        default="regs",
        help="C++ namespace to place all register groups in.",
    )
    parser.add_argument(
        "--naming",
        type=str,
        default="keep",
        choices=["keep", "lower", "upper"],
        help="How to represent names in the C++ code.",
    )
    parser.add_argument(
        "--output", type=str, required=True, help="Output file for generated C++ code."
    )
    parser.add_argument(
        "--parse-fn", type=str, help="Name of the parse function to use."
    )
    parser.add_argument(
        "--parser-modules",
        type=str,
        nargs="*",
        default=[],
        help="Path(s) to Python module(s) with parse function(s): parse_fn_name(filename: str) -> [Group].",
    )
    parser.add_argument(
        "--registers", type=str, nargs="+", default=[], help="Registers to generate."
    )

    return parser.parse_args()


def main():
    args = parse_cmdline()
    generate(args)


if __name__ == "__main__":
    main()
