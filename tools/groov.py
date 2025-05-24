from collections import namedtuple


Group = namedtuple("Group", ["name", "registers"])
Register = namedtuple("Register", ["name", "address", "fields"])
Field = namedtuple("Field", ["name", "access", "msb", "lsb"])
