# [roq-fix-bridge](https://roq-trading.com/docs/reference/clients/bridges/roq-fix-bridge/)

Roq's FIX bridge.


## Contact

Please reach out by [email](mailto:info@roq-trading.com) if you're interested in licensing this software.


## Documentation

Reference documentation can be found [here](https://roq-trading.com/docs/reference/clients/bridges/roq-fix-bridge/).


## Prerequisites

> Use `stable` for (the approx. monthly) release build.
> Use `unstable` for the more regularly updated development builds.

### Initialize sub-modules

```bash
git submodule update --init --recursive
```

### Create development environment

```bash
scripts/create_conda_env unstable debug
```

### Activate environment

```bash
source opt/conda/bin/activate dev
```


## Building

> Sometimes you may have to delete CMakeCache.txt if CMake has already cached an incorrect configuration.

```bash
cmake . && make -j4
```


## Links

* [Roq GmbH (website)](https://roq-trading.com/)
* [Contact (email)](mailto:info@roq-trading.com)
* [Documentation](https://roq-trading.com/docs/)
* [Releases](https://roq-trading.com/docs/releases/)
* [Gateways](https://roq-trading.com/docs/introduction/gateways/)
* [Samples](https://github.com/roq-trading/roq-cpp-samples/)
* [Pricing](https://roq-trading.com/#pricing)
* [LinkedIn](https://www.linkedin.com/company/35447832/)
* [Telegram](https://t.me/roq_trading/)


## License

The project is released under the terms of the BSD-3 license.
