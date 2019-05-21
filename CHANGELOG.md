# Changelog

## v0.2.1

* Bug fixes
  * Compile C code under `_build` rather than in the `src` directory

## v0.2.0

This change refactors option passing to the `private_key` helper function. Your
code can look like this now:

```elixir
NervesKey.PKCS11.private_key(engine, i2c: 1),
```

This allows the helper to take more than one option. It can differentiate
between primary and auxiliary keys now, but internally that doesn't make a
difference currently.

## v0.1.1

* Bug fixes
  * Improve robustness to ATECC508A errors by integrating improvements on other
    projects here

## v0.1.0

Initial release
