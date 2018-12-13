# NervesKey.PKCS11

[![CircleCI](https://circleci.com/gh/nerves-hub/nerves_key_pkcs11.svg?style=svg)](https://circleci.com/gh/nerves-hub/nerves_key_pkcs11)
[![Hex version](https://img.shields.io/hexpm/v/nerves_key_pkcs11.svg "Hex version")](https://hex.pm/packages/nerves_key_pkcs11)

This is a minimal implementation of [PKCS #11](https://en.wikipedia.org/wiki/PKCS_11)
for interacting with the NervesKey.  The NervesKey is a specific configuration
of the ATECC508A/ATECC608A chips that holds one private key in slot 0. If you're
using these chips in a similar configuration, this should work for you as well.

This library is organized to make it easy to integrate into Elixir. If you're not
using Elixir, you can still run `make` and copy `priv/nerves_key_pkcs11.so` to
a conveniently location.

Another option is to look at
[cryptoauth-openssl-engine](https://github.com/MicrochipTech/cryptoauth-openssl-engine)
or [cryptoauthlib](https://github.com/MicrochipTech/cryptoauthlib).

## Building

This library is self-contained with no compile-time dependencies. At runtime, it
isn't useful unless you have another program around that uses a PKCS #11 shared
library.

If using it with Elixir, add a dependency to your `mix.exs`:

```elixir
def deps do
  [
    {:nerves_key_pkcs11, "~> 0.1.0"}
  ]
end
```

If not using Elixir, run `make`. You may need to set $(CC) or $(CFLAGS) if you're
crosscompiling.

## OpenSSL integration

To use this with OpenSSL, you'll need `libpkcs11.so`. This library comes from
[OpenSC's libp11](https://github.com/OpenSC/libp11) and can be installed on
Debian systems by running:

```sh
sudo apt install libengine-pkcs11-openssl1.1
```

## Invocation from Erlang and Elixir

Erlang's `crypto` application provides an API for loading OpenSSL engines. See
[the Erlang crypto User's Guide](http://erlang.org/doc/apps/crypto/engine_load.html)
for details on this feature. `NervesKey.PKCS11.load_engine/0` is a helper method
to make the `:crypto.engine_load/3` call for you. It uses OpenSSL's `dynamic`
engine to load `libpkcs11.so` which in turn loads this PKCS #11 implementation.
Here's an example call in Elixir:

```elixir
{:ok, engine} = NervesKey.PKCS11.load_engine()
```

If this doesn't work, you'll likely have to look at the implentation of
`load_engine/0` and fine tune the shared library paths or control commands.

After you load the engine, you'll eventually want to use it. The intended use
case is for delegate the ECDSA operation to the ATECC508A for use with TLS
connections. You'll need to obtain the X.509 certificate that corresponds to the
private key held in the ATECC508A through some mechanism. Then in your SSL
options, you'll have something like this:

```elixir
[
  key: NervesKey.PKCS11.private_key(engine),
  certfile: "device-cert.pem",
]
```

The `NervesKey.PKCS11.private_key/1` helper method will create the appropriate
map so that Erlang's `:crypto` library can properly call into OpenSSL.

## License

The Elixir and most C code is licensed under the Apache License 2.0. See
`pkcs11.h` for its license.
