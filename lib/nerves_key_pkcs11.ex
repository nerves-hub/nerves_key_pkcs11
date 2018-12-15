defmodule NervesKey.PKCS11 do
  @moduledoc """
  This module contains helper methods for loading and using the PKCS #11
  module for NervesKey in Elixir. You don't need to use these methods to
  use the shared library.
  """

  @typedoc """
  The location of the NervesKey

  Currently only I2C bus locations are supported.
  """
  @type location :: {:i2c, 0..15}

  @doc """
  Load the OpenSSL engine
  """
  @spec load_engine() :: {:ok, :crypto.engine_ref()} | {:error, any()}
  def load_engine() do
    libpkcs = pkcs11_path()

    unless libpkcs do
      raise """
      Can't find libpkcs11.so. Please make sure that it is installed.
      """
    end

    # Load libpkcs11.so using OpenSSL's dynamic engine and then configure it
    :crypto.engine_load(
      "dynamic",
      [{"SO_PATH", libpkcs}, {"ID", "pkcs11"}, {"LOAD", ""}],
      [{"MODULE_PATH", Application.app_dir(:nerves_key_pkcs11, ["priv", "nerves_key_pkcs11.so"])}]
    )
  end

  @doc """
  Return the key map for passing a private key to ssl_opts

  This method creates the key map that the `:crypto` library can
  use to properly route private key operations to the PKCS #11
  shared library.
  """
  @spec private_key(:crypto.engine_ref(), location()) :: map()
  def private_key(engine, location) do
    %{
      algorithm: :ecdsa,
      engine: engine,
      key_id: "pkcs11:id=#{location_to_slot_id(location)}"
    }
  end

  defp location_to_slot_id({:i2c, bus_number}) when bus_number >= 0 and bus_number <= 16,
    do: bus_number

  defp pkcs11_path() do
    [
      "/usr/lib/engines-1.1/libpkcs11.so",
      "/usr/lib/x86_64-linux-gnu/engines-1.1/libpkcs11.so",
      "/usr/lib/engines/libpkcs11.so"
    ]
    |> Enum.find(&File.exists?/1)
  end
end
