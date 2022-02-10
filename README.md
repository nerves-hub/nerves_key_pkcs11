# NervesKey.PKCS11

[![CircleCI](https://circleci.com/gh/nerves-hub/nerves_key_pkcs11.svg?style=svg)](https://circleci.com/gh/nerves-hub/nerves_key_pkcs11)
[![Hex version](https://img.shields.io/hexpm/v/nerves_key_pkcs11.svg "Hex version")](https://hex.pm/packages/nerves_key_pkcs11)

This is a minimal implementation of [PKCS #11](https://en.wikipedia.org/wiki/PKCS_11)
for using the NervesKey with OpenSSL and other programs.  The NervesKey is a specific
configuration of the ATECC508A/ATECC608A chips that holds one private key in slot 0. If
you're using this chip in a similar configuration, this should work for you as well.

Supported features:

* ECDSA

This library is organized to make it easy to integrate into Elixir and is
written with an expectation that provisioning, extracting certificates, etc. is done
via other means (like using [nerves_key](https://hex.pm/nerves_key).) If you're not
using Elixir, you can still run `make` and copy `priv/nerves_key_pkcs11.so` to
a conveniently location. Elixir isn't needed to build the C library.

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
    {:nerves_key_pkcs11, "~> 1.0"}
  ]
end
```

If not using Elixir, run `make`. You may need to set $(CC) or $(CFLAGS) if you're
crosscompiling.

## Slot definition

PKCS #11 uses the term slot to refer to cryptographic devices. This library can
use either slot ID (if called directly) or the slot's token ID (if called via
libp11) to find the NervesKey. Various parameters are mapped into the slot ID
according to the table below:

Slot range  | I2C bus   | Bus address              | Certificate
------------|-----------|--------------------------|------------
0-15        | slot      | 0x60 (ATECC default)     | Primary or auxiliary
16-31       | slot-16   | 0x35 (Trust&Go versions) | Primary or auxiliary

On Linux, the I2C bus number in the table above determines the device file. For
example, a bus number of "1" maps to `/dev/i2c-1`. NervesKey devices support a
primary and auxiliary set of certificates. Normally only the primary device
certificate is used. Under some conditions, it's useful to write a second set of
certificates to the device and those can be referenced by using the "Auxiliary"
rows in the table.  This library currently need to differentiate between primary
and auxiliary certificates so that's not represented in the slot ID. It may be
in the future.

The [PKCS #11 URI](https://tools.ietf.org/html/rfc7512) for addressing the
desired NervesKey has the form:

```text
pkcs11:token=1
```

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

If this doesn't work, you'll likely have to look at the implementation of
`load_engine/0` and fine tune the shared library paths or control commands.

After you load the engine, you'll eventually want to use it. The intended use
case is for delegate the ECDSA operation to the ATECC508A for use with TLS
connections. You'll need to obtain the X.509 certificate that corresponds to the
private key held in the ATECC508A through some mechanism. Then in your SSL
options, you'll have something like this:

```elixir
[
  key: NervesKey.PKCS11.private_key(engine, i2c: 1, type: :nerves_key),
  certfile: "device-cert.pem",
]
```

The `NervesKey.PKCS11.private_key/2` helper method will create the appropriate
map so that Erlang's `:crypto` library can properly call into OpenSSL.

If you are using a pre-provisioned ATECC608B or similar that's labelled a Trust
and Go part, specify `:trust_and_go` for the `:type`.

## Sharing the NervesKey

If you have other code using the NervesKey, it might conflict with this library. There's
no lock file or mechanism to keep more than one process from accessing the ATECC508A
chip simultaneously. This is not expected to be an issue at runtime since the main
reason to access the NervesKey in another process is to provision it and that's not
something one would do when trying to use this library to assist a TLS negotiation.

## License

The Elixir and most C code is licensed under the 2-Clause BSD License.

The header file for the PKCS #11 function prototypes and structures, `pkcs11.h`,
has the following license:

```text
/* pkcs11.h
   Copyright 2006, 2007 g10 Code GmbH
   Copyright 2006 Andreas Jellinghaus

   This file is free software; as a special exception the author gives
   unlimited permission to copy and/or distribute it, with or without
   modifications, as long as this notice is preserved.

   This file is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY, to the extent permitted by law; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  */
```
