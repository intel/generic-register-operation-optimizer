import xml.etree.ElementTree as et
import re
from collections import namedtuple
import argparse


Group = namedtuple('Group', ['name', 'registers'])
Register = namedtuple('Register', ['name', 'address', 'fields'])
Field = namedtuple('Field', ['name', 'access', 'msb', 'lsb'])

def parse_svd(filename):
    def mk_field(x):
        (msb, lsb) = re.search(r"\[(\d+):(\d+)\]", x.find('bitRange').text).groups()

        return Field(
            name = x.find('name').text,
            access = x.find('access').text,
            msb = int(msb),
            lsb = int(lsb)
        )

    def mk_register(x, base):
        return Register(
            name = x.find('name').text,
            address = base + int(x.find('addressOffset').text, 16),
            fields = [mk_field(f) for f in x.findall('.//field')]
        )

    def mk_group(x, root):
        base_addr = int(x.find('baseAddress').text, 16)
        name = x.find('name').text

        if ("derivedFrom" in x.attrib):
            x = root.find(f""".//peripheral[name='{x.attrib["derivedFrom"]}']""")
            
        return Group(
            name = name,
            registers = [mk_register(r, base_addr) for r in x.findall('.//register')]
        )
    
    root = et.parse(filename).getroot()
    return [mk_group(p, root) for p in root.findall('.//peripheral')]


def generate_groups(groups, dest, namespace, name_func):
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
        return f"""groov::field<"{name_func(f.name)}", {integral_type}, {f.msb}, {f.lsb}>"""

    def generate_register(r):
        fields = indent([generate_field(f) for f in r.fields], len = 12)
        return f"""groov::reg<"{name_func(r.name)}", std::uint32_t, {hex(r.address)}{fields}>"""
    
    def generate_group(g):
        registers = indent([generate_register(r) for r in g.registers], len = 8)
        return f"""constexpr auto {name_func(g.name)} = \n    groov::group<"{name_func(g.name)}", groov::mmio_bus{registers}>{{}};\n"""

    for defn in groups:
        g = generate_group(defn)
        with open(f"{dest}/{name_func(defn.name)}.hpp", "w") as f:
            print("#include <groov.hpp>", file=f)
            print("#include <cstdint>", file=f)
            print("", file=f)
            print(f"namespace {namespace} {{", file=f)
            print(g, end="", file=f)
            print(f"}} // namespace {namespace}", file=f)

def parse_cmdline():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--input",
        type=str,
        required=True,
        help=(
            "Full path to SVD file with register definitions."
        ),
    )

    parser.add_argument(
        "--output", type=str, help="Output directory for generated C++ code."
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
        choices=['keep', 'lower', 'upper'],
        help="How to represent names in the C++ code.",
    )

    return parser.parse_args()

def name_func(choice):
    def to_lower(s):
        return s.lower()

    def to_upper(s):
        return s.upper()

    def keep(s):
        return s
    
    return dict(
        keep = keep,
        lower = to_lower,
        upper = to_upper
    )[choice]

def main():
    args = parse_cmdline()
    groups = parse_svd(args.input)
    generate_groups(groups, args.output, args.namespace, name_func(args.naming))

if __name__ == "__main__":
    main()
    