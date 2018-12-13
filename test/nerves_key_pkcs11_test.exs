defmodule NervesKey.PKCS11Test do
  use ExUnit.Case
  doctest NervesKey.PKCS11

  test "loads an engine" do
    {:ok, engine} = NervesKey.PKCS11.load_engine()
    assert is_reference(engine)
  end

  test "creates a key map" do
    engine = make_ref()
    key = NervesKey.PKCS11.private_key(engine)

    assert is_map(key)
    assert Map.get(key, :algorithm) == :ecdsa
    assert Map.get(key, :engine) == engine
    assert Map.get(key, :key_id) == "pkcs11:id=0;type=private"
  end
end
