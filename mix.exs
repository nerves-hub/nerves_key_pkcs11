defmodule NervesKey.PKCS11.MixProject do
  use Mix.Project

  def project do
    [
      app: :nerves_key_pkcs11,
      version: "0.1.0",
      elixir: "~> 1.7",
      description: description(),
      package: package(),
      compilers: [:elixir_make | Mix.compilers()],
      make_clean: ["clean"],
      docs: [extras: ["README.md"]],
      start_permanent: Mix.env() == :prod,
      deps: deps()
    ]
  end

  def application, do: []

  defp description do
    "PKCS #11 module for using the NervesKey"
  end

  defp package do
    %{
      files: [
        "lib",
        "src/*.[ch]",
        "mix.exs",
        "README.md",
        "LICENSE",
        "Makefile"
      ],
      licenses: ["BSD-2-Clause"],
      links: %{"GitHub" => "https://github.com/nerves-hub/nerves_key_pkcs11"}
    }
  end

  defp deps do
    [
      {:elixir_make, "~> 0.4", runtime: false},
      {:ex_doc, "~> 0.19", only: [:dev, :test], runtime: false},
      {:dialyxir, "1.0.0-rc.4", only: :dev, runtime: false}
    ]
  end
end
