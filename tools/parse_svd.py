def parse_svd(filename):
    import groov
    import re
    import xml.etree.ElementTree as et

    def mk_field(x):
        (msb, lsb) = re.search(r"\[(\d+):(\d+)\]", x.find("bitRange").text).groups()

        return groov.Field(
            name=x.find("name").text,
            access=x.find("access").text,
            msb=int(msb),
            lsb=int(lsb),
        )

    def mk_register(x, base):
        return groov.Register(
            name=x.find("name").text,
            address=base + int(x.find("addressOffset").text, 16),
            fields=[mk_field(f) for f in x.findall(".//field")],
        )

    def mk_group(x, root):
        base_addr = int(x.find("baseAddress").text, 16)
        name = x.find("name").text

        if "derivedFrom" in x.attrib:
            x = root.find(f""".//peripheral[name='{x.attrib["derivedFrom"]}']""")

        return groov.Group(
            name=name,
            registers=[mk_register(r, base_addr) for r in x.findall(".//register")],
        )

    root = et.parse(filename).getroot()
    return [mk_group(p, root) for p in root.findall(".//peripheral")]
