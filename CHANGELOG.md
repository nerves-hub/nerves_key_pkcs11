# Changelog

## v1.0.0

This release only bumps the version number. It doesn't have any code changes.

## v0.2.4

* Bug fixes
  * Fix segfault with libp11 v0.4.11

## v0.2.3

* Bug fixes
  * Change pkcs11 URL to specify `token`. See
    https://github.com/nerves-hub/nerves_key_pkcs11/pull/12.

## v0.2.2

* Bug fixes
  * Add `:crypto` to the dependencies to fix Elixir 1.11 warning.

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
