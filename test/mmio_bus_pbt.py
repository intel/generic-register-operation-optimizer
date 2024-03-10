from hypothesis import given, settings, strategies as st
import cppyy

cppyy.add_include_path("include")
cppyy.add_include_path("test")
cppyy.add_include_path("build/_deps/cpp-baremetal-concurrency-src/include")
cppyy.add_include_path("build/_deps/cpp-std-extensions-src/include")
cppyy.add_include_path("build/_deps/cpp-baremetal-senders-and-receivers-src/include")
cppyy.add_include_path("build/_deps/mp11-src/include")

cppyy.include("mmio_bus_pbt.hpp")

from cppyy.gbl import test

def uints(size):
    return st.integers(min_value=0, max_value=2**size-1)

def do_mmio_bus_write(type, initial, mask, id_mask, id_value, write_value):
    write_value = mask & write_value
    id_mask = ~mask & id_mask
    id_value = id_mask & id_value
    initial = (~id_mask & initial) | id_value

    test.reg64 = initial

    test.bus_write[type, mask, id_mask, id_value](test.addr64, write_value)

    assert ((mask      & test.reg64) == (mask & write_value))
    assert ((id_mask   & test.reg64) == (id_value))

    keep_mask = ~mask & ~id_mask
    assert ((keep_mask & test.reg64) == (keep_mask & initial))

@settings(max_examples=1000)
@given(uints(64), uints(64), uints(64), uints(64), uints(64))
def test_mmio_bus_writes_with_id_mask_64(initial, mask, id_mask, id_value, write_value):
    do_mmio_bus_write("std::uint64_t", initial, mask, id_mask, id_value, write_value)

@settings(max_examples=1000)
@given(uints(32), uints(32), uints(32), uints(32), uints(32))
def test_mmio_bus_writes_with_id_mask_32(initial, mask, id_mask, id_value, write_value):
    do_mmio_bus_write("std::uint32_t", initial, mask, id_mask, id_value, write_value)

@settings(max_examples=1000)
@given(uints(16), uints(16), uints(16), uints(16), uints(16))
def test_mmio_bus_writes_with_id_mask_16(initial, mask, id_mask, id_value, write_value):
    do_mmio_bus_write("std::uint16_t", initial, mask, id_mask, id_value, write_value)

@settings(max_examples=1000)
@given(uints(8), uints(8), uints(8), uints(8), uints(8))
def test_mmio_bus_writes_with_id_mask_8(initial, mask, id_mask, id_value, write_value):
    do_mmio_bus_write("std::uint8_t", initial, mask, id_mask, id_value, write_value)

