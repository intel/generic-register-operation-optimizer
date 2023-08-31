# Generic Register Operation Optimzier 5 (groov)

## WARNING: Work in Progress!

This library is being developed in the open. It is considered a work in progress
and not ready for production use. It may be buggy, it may not do what it says,
it may not even have any source code at this moment.

## Introduction

_groov_ is an open source hardware register abstraction library. It is designed
to provide a safe, efficient, and maintainable interface to hardware registers
in firmware.

## Quickstart

### Define Bus Interface

```c++
constexpr struct mmio_bus_t : public groov::bus<uint32_t, uint32_t> {
  static async::sender auto read(uint32_t addr);
  static async::sender auto write(uint32_t addr, auto mask, uint32_t data);

  template<uint32_t mask> static constexpr auto to_bus_mask_v;
  template<auto mask> static constexpr uint32_t DataType to_raw_mask_v;
} mmio_bus;
```

### Define Hardware Registers

```c++
constexpr auto mailbox = 
	groov::group<"mailbox", mmio_bus,
		groov::reg<"cmd", 0xfe0'0000,
			groov::field<"start", 0,  0, groov::w1s>,
			groov::field<"length", 18, 16, groov::rw>,
			groov::field<"opcode", 31, 24, groov::rw>
		>,
		groov::reg<"payload0", 0xfe0'0004>,
		groov::reg<"payload1", 0xfe0'0008>,
		groov::reg<"payload2", 0xfe0'000c>,
		groov::reg<"payload3", 0xfe0'0010>,
		groov::reg<"payload4", 0xfe0'0014>,
		groov::reg<"payload5", 0xfe0'0018>,
		groov::reg<"payload6", 0xfe0'001c>,
		groov::reg<"payload7", 0xfe0'0020>
	>;
```

Banked registers can be used to efficiently replicate contiguous registers with
identical field definitions. 

```c++
constexpr auto mailbox = 
	groov::group<"mailbox", mmio_bus,
		groov::reg<"cmd", 0xfe0'0000,
			groov::field<"start", 0,  0, groov::w1s>,
			groov::field<"length", 18, 16, groov::rw>,
			groov::field<"opcode", 31, 24, groov::rw, opcode_t>
		>,

        // TODO: need support for multi-dimensional indexing of banked registers
		groov::banked_reg<"payload", 0xfe00'0004, 32, 4>

        // TODO: need support for aliasing registers?
	>;
```

### Use Registers

```c++

// write some registers using flat syntax (like croo)
// NOTE: registers can be treated same as fields
auto send_msg(opcode_t op, uint32_t data) -> void {
    sync_wait(apply(write(
        mailbox / "payload"_r[0] = data
    )));

    sync_wait(apply(write(
        mailbox / "cmd.start"_f = 1,
        mailbox / "cmd.length"_f = 1,
        mailbox / "cmd.opcode"_f = op
    )));
}

// write some registers using new hierarchical syntax
auto send_msg(opcode_t op, uint32_t data) -> void {
    sync_wait(apply(
        mailbox(
            "cmd"_r(
                "start"_f = 1,
                "length"_f = 1,
                "opcode"_f = op
            ),
            "payload"_r[0] = data,
            "status"_r
        )
    ));

    sync_wait(apply(write(
        mailbox(
            "cmd"_r(
                "start"_f = 1,
                "length"_f = 1,
                "opcode"_f = [](auto old_op){
                    return old_op + 1;
                }
            ),
            "payload"_r[0] = data
        )
    )));
}

// read a register
apply(mailbox / "cmd"_r) | async::then([](auto cmd){
    auto op = cmd["opcode"_f];
});

// write a single register field
apply(mailbox / "cmd.start"_f = 1) | async::then(...);

// shortcut for blocking operations
auto cmd = apply_now(mailbox / "cmd"_r);
auto op = cmd["opcode"_f];

apply_now(mailbox / "cmd.start"_f = 1);
```


### Create and Use Temporaries

```c++
using cmd_t = groov::temp<mailbox / "cmd"_r>;

cmd_t my_temp_cmd{"start"_f = 1, "opcode"_f = opcode_t::FETCH_PATCH};
my_temp_cmd["length"_f] = 0;

apply(my_temp_cmd) | async::then(...);

apply_now(my_temp_cmd);
```

## Testing

### Create a Mock Register Group

`groov` mocks out registers by name, not address. This enables `groov` to
automatically create mock register groups for you.

```c++
groov::mock_group mailbox{};
```

Creating expectations on mocks is straightforward.

```c++
GROOV_EXPECT_WRITE(
    mailbox(
        "cmd"_r(
            "start"_f = 1, 
            "opcode"_f = opcode_t::FETCH_PATCH, 
            "length"_f = 0
        )
    )
);

GROOV_EXPECT_READ(mailbox / "cmd"_r = 0);
```
