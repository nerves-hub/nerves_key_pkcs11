# SPDX-FileCopyrightText: 2018 Frank Hunleth
# SPDX-FileCopyrightText: 2022 Connor Rigby
#
# SPDX-License-Identifier: BSD-2-Clause
#
defmodule NervesKey.PKCS11Test do
  use ExUnit.Case
  doctest NervesKey.PKCS11

  test "loads an engine" do
    {:ok, engine} = NervesKey.PKCS11.load_engine()
    assert is_reference(engine)
  end

  test "can load an engine twice" do
    # This tests the logic that prevents multiple loads if using libopenssl
    # 1.1.1m or later.  Those versions of libopenssl will fail the second
    # engine load if attempted.
    {:ok, engine} = NervesKey.PKCS11.load_engine()
    assert is_reference(engine)

    {:ok, engine2} = NervesKey.PKCS11.load_engine()
    assert is_reference(engine2)
  end

  test "creates a key map" do
    engine = make_ref()
    key = NervesKey.PKCS11.private_key(engine, i2c: 0)

    assert is_map(key)
    assert Map.get(key, :algorithm) == :ecdsa
    assert Map.get(key, :engine) == engine
    assert Map.get(key, :key_id) == "pkcs11:token=0"
  end

  test "maps I2C locations" do
    engine = make_ref()
    key = NervesKey.PKCS11.private_key(engine, i2c: 1)
    assert Map.get(key, :key_id) == "pkcs11:token=1"

    key = NervesKey.PKCS11.private_key(engine, i2c: 3)
    assert Map.get(key, :key_id) == "pkcs11:token=3"
  end

  test "accepts aux and primary" do
    # These don't do anything now, but we may need them in the future.
    engine = make_ref()
    key = NervesKey.PKCS11.private_key(engine, i2c: 1, certificate: :aux)
    assert Map.get(key, :key_id) == "pkcs11:token=1"

    key = NervesKey.PKCS11.private_key(engine, i2c: 1, certificate: :primary)
    assert Map.get(key, :key_id) == "pkcs11:token=1"
  end

  test "supports old option format for the time being" do
    engine = make_ref()
    key = NervesKey.PKCS11.private_key(engine, {:i2c, 0})
    assert Map.get(key, :key_id) == "pkcs11:token=0"
  end

  test "trust and go" do
    engine = make_ref()
    key = NervesKey.PKCS11.private_key(engine, i2c: 0, type: :trust_and_go)
    assert Map.get(key, :key_id) == "pkcs11:token=16"

    key = NervesKey.PKCS11.private_key(engine, i2c: 1, type: :trust_and_go)
    assert Map.get(key, :key_id) == "pkcs11:token=17"
  end
end
