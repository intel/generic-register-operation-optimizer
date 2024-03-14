from hypothesis import given, settings, strategies as st

def uints(size):
    return st.integers(min_value=0, max_value=2**size-1)

def do_mmio_bus_write(test, type, initial, mask, id_mask, id_value, write_value):
    write_value = mask & write_value
    id_mask = ~mask & id_mask
    id_value = id_mask & id_value
    initial = (~id_mask & initial) | id_value

    from cppyy.gbl import test
    test.reg64 = initial

    test.bus_write[type, mask, id_mask, id_value](test.addr64, write_value)

    assert ((mask      & test.reg64) == (mask & write_value))
    assert ((id_mask   & test.reg64) == (id_value))

    keep_mask = ~mask & ~id_mask
    assert ((keep_mask & test.reg64) == (keep_mask & initial))

@settings(deadline=None)
@given(uints(64), uints(64), uints(64), uints(64), uints(64))
def test_mmio_bus_writes_with_id_mask_64(cppyy_test, initial, mask, id_mask, id_value, write_value):
    do_mmio_bus_write(cppyy_test, "std::uint64_t", initial, mask, id_mask, id_value, write_value)

@settings(deadline=None)
@given(uints(32), uints(32), uints(32), uints(32), uints(32))
def test_mmio_bus_writes_with_id_mask_32(cppyy_test, initial, mask, id_mask, id_value, write_value):
    do_mmio_bus_write(cppyy_test, "std::uint32_t", initial, mask, id_mask, id_value, write_value)

@settings(deadline=None)
@given(uints(16), uints(16), uints(16), uints(16), uints(16))
def test_mmio_bus_writes_with_id_mask_16(cppyy_test, initial, mask, id_mask, id_value, write_value):
    do_mmio_bus_write(cppyy_test, "std::uint16_t", initial, mask, id_mask, id_value, write_value)

@settings(deadline=None)
@given(uints(8), uints(8), uints(8), uints(8), uints(8))
def test_mmio_bus_writes_with_id_mask_8(cppyy_test, initial, mask, id_mask, id_value, write_value):
    do_mmio_bus_write(cppyy_test, "std::uint8_t", initial, mask, id_mask, id_value, write_value)

