#ifndef RTC
#define RTC

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <array>

class RTCClock {
public:
    static std::string getTime() {
        return executeCommand("hwclock --get");
    }

    static std::string setTime(const std::string& timeStr) {
        std::string command = "hwclock --set --date=\"" + timeStr + "\"";
        return executeCommand(command);
    }

    static std::string syncToSystem() {
        return executeCommand("hwclock --systohc");
    }

    static std::string syncToRTC() {
        return executeCommand("hwclock --hctosys");
    }

    static std::string printRTCTime() {
        return getTime();
    }

private:
    RTCClock() = default;

    static std::string executeCommand(const std::string& command) {
        std::array<char, 128> buffer;
        std::string result;
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        try {
            while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
                result += buffer.data();
            }
        } catch (...) {
            pclose(pipe);
            throw;
        }
        pclose(pipe);
        return result;
    }
};

#endif 

