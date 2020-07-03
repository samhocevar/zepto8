# z8tool

**z8tool** is a multi-purpose tool for working with PICO-8 cartridges.

Usage: `z8tool <command> <arguments>`

## `z8tool stats`

Outputs statistics about a cart.

Usage:

    z8tool stats <cart>

Where `<cart>` is any cartridge in P8 (`.p8`), PNG (`.p8.png`), JavaScript (`.js`) or binary format (`.bin`).

Example:

```
% z8tool stats celeste.p8
file_name: celeste.p8
token_count: 5921 [8192]
code_size: 27484 [65535]
compressed_code_size: 7903 [15616]

%
```

## `z8tool listlua`

Extract code from a cart, in text format.

Usage:

    z8tool listlua <cart>

Where `<cart>` is any cartridge in P8 (`.p8`), PNG (`.p8.png`), JavaScript (`.js`) or binary format (`.bin`).

## `z8tool printast`

Not fully implemented yet.

## `z8tool convert`

Convert carts beetween formats: P8 (`.p8`), PNG (`.p8.png`), JavaScript (`.js`) or binary format (`.bin`).

Usage:

    z8tool convert <input> <output>

Examples:

    % z8tool convert celeste.p8.png celeste.p8
    % z8tool convert celeste.p8 other_celeste.p8.png
    %

## `z8tool run`

Run a cart in the terminal.

Usage:

    z8tool run [--telnet] [--headless] <cart>

  - `--telnet` emit telnet server commands, for use with socat
  - `--headless` run without displaying anything

## `z8tool dither`

Not fully implemented yet.

## `z8tool compress`

Not fully implemented yet.

## `z8tool test`

Run the internal test suite.  Not fully implemented yet.

