#include "DeviceIdentity.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <sys/utsname.h>
#include <vector>

namespace RomCloud {

namespace {

constexpr std::array<uint32_t, 64> SHA256_CONSTANTS = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

uint32_t rotateRight(uint32_t value, uint32_t bits) {
    return (value >> bits) | (value << (32U - bits));
}

class Sha256Hasher {
public:
    void update(const uint8_t* data, size_t length) {
        m_totalBytes += length;
        while (length > 0) {
            const size_t available = m_buffer.size() - m_bufferSize;
            const size_t copyLength = std::min(length, available);
            std::copy(data, data + copyLength, m_buffer.begin() + m_bufferSize);
            m_bufferSize += copyLength;
            data += copyLength;
            length -= copyLength;
            if (m_bufferSize == m_buffer.size()) {
                transform(m_buffer.data());
                m_bufferSize = 0;
            }
        }
    }

    std::string finish() {
        const uint64_t bitLength = m_totalBytes * 8U;
        const uint8_t marker = 0x80U;
        update(&marker, 1U);
        const uint8_t zero = 0U;
        while (m_bufferSize != 56U) update(&zero, 1U);

        std::array<uint8_t, 8> encodedLength{};
        for (size_t index = 0; index < encodedLength.size(); ++index) {
            encodedLength[index] = static_cast<uint8_t>(bitLength >> ((7U - index) * 8U));
        }
        update(encodedLength.data(), encodedLength.size());

        std::ostringstream output;
        output << std::hex << std::setfill('0');
        for (uint32_t word : m_hash) output << std::setw(8) << word;
        return output.str();
    }

private:
    void transform(const uint8_t* block) {
        std::array<uint32_t, 64> words{};
        for (size_t index = 0; index < 16U; ++index) {
            const size_t position = index * 4U;
            words[index] = (static_cast<uint32_t>(block[position]) << 24U) |
                           (static_cast<uint32_t>(block[position + 1U]) << 16U) |
                           (static_cast<uint32_t>(block[position + 2U]) << 8U) |
                           static_cast<uint32_t>(block[position + 3U]);
        }
        for (size_t index = 16U; index < words.size(); ++index) {
            const uint32_t sigma0 = rotateRight(words[index - 15U], 7U) ^
                                    rotateRight(words[index - 15U], 18U) ^
                                    (words[index - 15U] >> 3U);
            const uint32_t sigma1 = rotateRight(words[index - 2U], 17U) ^
                                    rotateRight(words[index - 2U], 19U) ^
                                    (words[index - 2U] >> 10U);
            words[index] = words[index - 16U] + sigma0 + words[index - 7U] + sigma1;
        }

        uint32_t a = m_hash[0];
        uint32_t b = m_hash[1];
        uint32_t c = m_hash[2];
        uint32_t d = m_hash[3];
        uint32_t e = m_hash[4];
        uint32_t f = m_hash[5];
        uint32_t g = m_hash[6];
        uint32_t h = m_hash[7];
        for (size_t index = 0; index < words.size(); ++index) {
            const uint32_t sum1 = rotateRight(e, 6U) ^ rotateRight(e, 11U) ^ rotateRight(e, 25U);
            const uint32_t choice = (e & f) ^ ((~e) & g);
            const uint32_t temporary1 = h + sum1 + choice + SHA256_CONSTANTS[index] + words[index];
            const uint32_t sum0 = rotateRight(a, 2U) ^ rotateRight(a, 13U) ^ rotateRight(a, 22U);
            const uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t temporary2 = sum0 + majority;
            h = g;
            g = f;
            f = e;
            e = d + temporary1;
            d = c;
            c = b;
            b = a;
            a = temporary1 + temporary2;
        }
        m_hash[0] += a;
        m_hash[1] += b;
        m_hash[2] += c;
        m_hash[3] += d;
        m_hash[4] += e;
        m_hash[5] += f;
        m_hash[6] += g;
        m_hash[7] += h;
    }

    std::array<uint32_t, 8> m_hash = {
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U
    };
    std::array<uint8_t, 64> m_buffer{};
    size_t m_bufferSize = 0;
    uint64_t m_totalBytes = 0;
};

std::string trim(std::string value) {
    value.erase(std::remove(value.begin(), value.end(), '\0'), value.end());
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string compactIdentity(const std::string& value) {
    std::string compact;
    for (unsigned char ch : value) {
        if (std::isalnum(ch)) compact += static_cast<char>(std::tolower(ch));
    }
    return compact;
}

bool isUsableIdentity(const std::string& value) {
    const std::string compact = compactIdentity(value);
    return !compact.empty() && compact.find_first_not_of('0') != std::string::npos &&
           compact != "ffffffffffff";
}

std::string firstIdentity(const std::vector<std::string>& paths) {
    for (const auto& path : paths) {
        std::string value = DeviceIdentity::readIdentityFile(path);
        if (path == "/sys/class/sunxi_info/sys_info") {
            std::smatch match;
            if (std::regex_search(value, match,
                    std::regex("sunxi_chipid\\s*:\\s*([0-9a-fA-F]+)", std::regex::icase))) {
                value = match[1].str();
            } else {
                value.clear();
            }
        }
        value = trim(value);
        if (isUsableIdentity(value)) return value;
    }
    return "";
}

} // namespace

std::string DeviceIdentity::readIdentityFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";
    std::string value(512, '\0');
    file.read(&value[0], static_cast<std::streamsize>(value.size()));
    value.resize(static_cast<size_t>(file.gcount()));
    return trim(value);
}

std::string DeviceIdentity::sha256Hex(const std::string& value) {
    Sha256Hasher hasher;
    hasher.update(reinterpret_cast<const uint8_t*>(value.data()), value.size());
    return hasher.finish();
}

std::string DeviceIdentity::sha256File(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";
    Sha256Hasher hasher;
    std::array<char, 64 * 1024> buffer{};
    while (file) {
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize bytesRead = file.gcount();
        if (bytesRead > 0) {
            hasher.update(reinterpret_cast<const uint8_t*>(buffer.data()),
                          static_cast<size_t>(bytesRead));
        }
    }
    return file.eof() ? hasher.finish() : "";
}

std::string DeviceIdentity::hardwareId() {
    const std::vector<std::string> serialPaths = {
        "/sys/firmware/devicetree/base/serial-number",
        "/proc/device-tree/serial-number",
        "/sys/devices/soc0/serial_number",
        "/sys/class/sunxi_info/sys_info",
        "/sys/class/dmi/id/product_uuid"
    };
    const std::vector<std::string> macPaths = {
        "/sys/class/net/wlan0/perm_address",
        "/sys/class/net/wlan0/address",
        "/sys/class/net/wlan1/perm_address",
        "/sys/class/net/wlan1/address"
    };
    const std::vector<std::string> machineIdPaths = {
        "/etc/machine-id",
        "/var/lib/dbus/machine-id"
    };

    std::string kind = "serial";
    std::string identity = firstIdentity(serialPaths);
    if (identity.empty()) {
        kind = "mac";
        identity = firstIdentity(macPaths);
    }
    if (identity.empty()) {
        kind = "machine";
        identity = firstIdentity(machineIdPaths);
    }
    if (identity.empty()) {
        struct utsname info{};
        uname(&info);
        kind = "fallback";
        identity = std::string(info.machine) + "|" + deviceModel();
    }

    const std::string material = "romcloud-device-v1|" + kind + "=" + identity;
    std::string digest = sha256Hex(material).substr(0, 12);
    std::transform(digest.begin(), digest.end(), digest.begin(), ::toupper);
    return "HW-" + digest;
}

std::string DeviceIdentity::deviceModel() {
    const std::vector<std::string> modelPaths = {
        "/sys/firmware/devicetree/base/model",
        "/proc/device-tree/model",
        "/sys/devices/soc0/machine",
        "/etc/trimui_device.txt"
    };
    std::string model = firstIdentity(modelPaths);
    if (model.empty()) {
        struct utsname info{};
        if (uname(&info) == 0) model = info.machine;
    }
    model = std::regex_replace(model, std::regex("[^A-Za-z0-9 ._()+/-]+"), " ");
    model = trim(model);
    if (model.size() > 80) model.resize(80);
    return model.empty() ? "Unknown TrimUI" : model;
}

} // namespace RomCloud
