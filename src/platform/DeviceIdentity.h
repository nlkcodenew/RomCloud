#pragma once

#include <string>

namespace RomCloud {

class DeviceIdentity {
public:
    static std::string hardwareId();
    static std::string deviceModel();
    static std::string sha256Hex(const std::string& value);
    static std::string readIdentityFile(const std::string& path);
};

} // namespace RomCloud
