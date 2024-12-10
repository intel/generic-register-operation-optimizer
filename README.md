# Generic Register Operation Optimizer 5 (groov)

## WARNING: Work in Progress!

This library is being developed in the open. It is considered a work in progress
and not ready for production use. It may be buggy, it may not do what it says,
it may not even have any source code at this moment.

## Introduction

`groov` is an open source hardware register abstraction library. It is designed
to provide a safe, efficient, and maintainable interface to hardware registers
in firmware.

### Asynchronous Support

`groov` presents an asynchronous interface for accessing registers. Hardware
registers are not always local to the processor's memory bus, they could be 
on a remote device over I2C, SPI, CAN, or another interface with a non-trivial
latency. An asynchronous interface is key to robustly and efficiently accessing
registers on remote busses.

`groov` uses [Intel's baremetal senders and recievers library](https://github.com/intel/cpp-baremetal-senders-and-receivers) to
provide a robust asynchronous execution framework. 

### Optimized Operations

Hardware registers often contain multiple fields packed in a single word. Each
field may have different access attributes: read/write, read-only, write-only,
write 1 to clear, write 1 to set, etc. When writing a subset of the fields of
a register, it is often necessary to preserve the values of the remaining 
fields. `groov` calculates whether a read-modify-write is necessary based on
the field attributes, as well as the capabilities of the bus the field is on.

If the bus can access a subset of the register, either through explicit byte-
enables or sub-word access, then `groov` will perform this special access if
it means a read-modify-write can be avoided.

## Quickstart

### Include groov

```c++
#include <groov/groov.hpp>
```

### Read a Register

```c++
// read a register field synchronously
auto op = async::sync_wait(groov::read(mbox / "cmd.opcode"_f));

// read a whole register synchronously
auto cmd = async::sync_wait(groov::read(mbox / "cmd"_r));

// access fields of read result
auto op = cmd["opcode"_f];
auto len = cmd["length"_f];
```

### Write a Register

```c++
// write a register
async::sync_wait(
    async::just(mbox / "payload"_r[0] = 0xcafed00d) |
    groov::write
);

// write multiple fields in a register
async::sync_wait(
    async::just(mbox / "cmd"_r(
        "start"_f = 1,
        "length"_f = 1,
        "opcode"_f = opcode_t::MSG
    )) |
    groov::write
);
```

### Read-modify-write Operation

```c++
async::sync_wait(
    groov::read(mbox / "cmd.length"_f) |
    async::then([](auto r){
        if (r["cmd.length"_f] < 10) {
            r["cmd.length"_f]++;
        }

        return r;
    }) |
    groov::write
);
```
### Define Hardware Registers

Registers are defined declaratively using a simple DSL. Register groups contain
registers and registers contain fields. Registers and fields are referenced
using compile-time strings.

```c++
constexpr auto mbox = 
    groov::group<"mbox", mmio_bus,
        groov::reg<"cmd", 0xfe0'0000, uint32_t,
            groov::field<"start", 0,  0, groov::w1s>,
            groov::field<"length", 19, 16, groov::rw>,
            groov::field<"opcode", 31, 24, groov::rw, opcode_t>
        >,

        groov::banked_reg<"payload", 0xfe00'0004, uint32_t>
    >;
```

### Define Bus Interface

`groov` needs to be told how registers in a group can be accessed. The 
`groov::bus` concept provides this mechanism.

```c++
constexpr struct mmio_bus_t {
  static async::sender auto read(uint32_t addr);
  static async::sender auto write(uint32_t addr, auto mask, uint32_t data);

  template<uint32_t mask> static constexpr auto to_bus_mask_v;
  template<auto mask> static constexpr uint32_t DataType to_raw_mask_v;
} mmio_bus;
```

### Temporary Register Values

Temporary register values are a fundamental concept of `groov`. They are used
for reading, writing, and passing register values around. 

If you've been reading this document, then you've already seen them in action.
Below you can see how they are used outside of a `read` or `write` operation.

```c++
// create a temporary of the 'cmd' register.
auto cmd = mbox / "cmd"_r;
cmd["opcode"_f] = opcode_t::CMD;

// writes 'opcode' to CMD, and remaining fields in cmd register to '0'
async::sync_wait(async::just(cmd) | groov::write);

// create a temporary of just the 'cmd.opcode' field.
auto op = mbox / "cmd.opcode"_f;
op = opcode_t::MSG;

// only the opcode field will be updated
// (depending on the bus, a read-modify-write operation may be executed)
async::sync_wait(async::just(op) | groov::write);
```

## Testing

### Create a Mock Register Group

`groov` mocks out registers by name, not address. This enables `groov` to
automatically create mock register groups for you.

```c++
groov::mock_group mbox{};
```

Creating expectations on mocks is straightforward.

```c++
GROOV_EXPECT_WRITE(
    mbox(
        "cmd"_r(
            "start"_f = 1, 
            "opcode"_f = opcode_t::FETCH_PATCH, 
            "length"_f = 0
        )
    )
);

GROOV_EXPECT_READ(mbox / "cmd"_r = 0);
```
