#pragma once
#include <string>
#include <vector>
#include <optional>

namespace kvm::core {

class CommandLineParser {
public:
    CommandLineParser(int argc, char* argv[]) {
        for (int i = 1; i < argc; ++i) {
            m_args.push_back(argv[i]);
        }
    }

    std::optional<std::string> GetOption(const std::string& name) const {
        for (size_t i = 0; i < m_args.size(); ++i) {
            if (m_args[i] == name && i + 1 < m_args.size()) {
                return m_args[i + 1];
            }
        }
        return std::nullopt;
    }

private:
    std::vector<std::string> m_args;
};

} // namespace kvm::core
