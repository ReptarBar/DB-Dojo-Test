#pragma once

#include "duckdb/main/extension.hpp"
#include "duckdb/main/extension_util.hpp"

namespace duckdb {

class DojoExtension : public Extension {
public:
	void Load(ExtensionLoader &loader) override;
	std::string Name() override;
	std::string Version() const override;
};

} // namespace duckdb
