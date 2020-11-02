defmodule NervesKey.PKCS11.MixProject do
  use Mix.Project

  def project do
    [
      app: :nerves_key_pkcs11,
      version: "0.2.1",
      elixir: "~> 1.7",
      description: description(),
      package: package(),
      compilers: [:elixir_make | Mix.compilers()],
      make_targets: ["all"],
      make_clean: ["clean"],
      dialyzer: dialyzer(),
      docs: [extras: ["README.md"]],
      start_permanent: Mix.env() == :prod,
      deps: deps()
    ]
  end

  def application do
    [extra_applications: [:crypto]]
  end

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
      {:ex_doc, "~> 0.19", only: :docs, runtime: false},
      {:dialyxir, "~> 1.0.0", only: :dev, runtime: false}
    ]
  end

  defp dialyzer() do
    [
      flags: [:race_conditions, :unmatched_returns, :error_handling]
    ]
  end
end
