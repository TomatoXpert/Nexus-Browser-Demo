
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdlib>
#include <csignal>
#include <thread>
#include <chrono>
#include <fstream>

namespace fs = std::filesystem;

// Simple logger
void log(const std::string& msg) {
    std::ofstream logfile("vpn_client.log", std::ios_base::app);
    logfile << "[" << std::time(nullptr) << "] " << msg << std::endl;
}

// List config files for a protocol
std::vector<fs::path> list_configs(const std::string& protocol, const std::string& config_dir) {
    std::vector<fs::path> configs;
    for (const auto& entry : fs::directory_iterator(config_dir)) {
        if (protocol == "OpenVPN" && entry.path().extension() == ".ovpn") {
            configs.push_back(entry.path());
        }
        if (protocol == "WireGuard" && entry.path().extension() == ".conf") {
            configs.push_back(entry.path());
        }
    }
    return configs;
}

// Global process PID
pid_t child_pid = -1;

// Signal handler for Ctrl+C
void signal_handler(int signum) {
    if (child_pid > 0) {
        std::cout << "\nDisconnecting..." << std::endl;
        kill(child_pid, SIGTERM);
        log("Disconnected VPN (terminated child process)");
    }
    std::exit(0);
}

int main() {
    std::string config_dir = "./vpn_configs";
    if (!fs::exists(config_dir) || !fs::is_directory(config_dir)) {
        std::cerr << "Config directory " << config_dir << " does not exist. Please create and add config files." << std::endl;
        return 1;
    }

    std::cout << "=== Nexus VPN Client (C++) ===" << std::endl;

    signal(SIGINT, signal_handler);

    while (true) {
        std::cout << "\nSelect protocol:\n1. OpenVPN\n2. WireGuard\n3. Exit\n> ";
        int proto_choice;
        std::cin >> proto_choice;
        std::cin.ignore();
        std::string protocol;
        if (proto_choice == 1) protocol = "OpenVPN";
        else if (proto_choice == 2) protocol = "WireGuard";
        else break;

        auto configs = list_configs(protocol, config_dir);
        if (configs.empty()) {
            std::cout << "No " << protocol << " config files found in " << config_dir << std::endl;
            continue;
        }
        std::cout << "Available servers:" << std::endl;
        for (size_t i = 0; i < configs.size(); ++i)
            std::cout << i+1 << ". " << configs[i].filename() << std::endl;
        std::cout << "Select server [1-" << configs.size() << "]: ";
        size_t cfg_idx;
        std::cin >> cfg_idx;
        std::cin.ignore();
        if (cfg_idx < 1 || cfg_idx > configs.size()) {
            std::cout << "Invalid selection." << std::endl;
            continue;
        }
        std::string cfg_path = configs[cfg_idx-1].string();

        std::cout << "Connecting to " << configs[cfg_idx-1].filename() << "..." << std::endl;
        log("Connecting to " + configs[cfg_idx-1].filename().string());

        if (protocol == "OpenVPN") {
            child_pid = fork();
            if (child_pid == 0) {
                execlp("sudo", "sudo", "openvpn", "--config", cfg_path.c_str(), (char*) nullptr);
                std::cerr << "Failed to exec openvpn!" << std::endl;
                std::exit(1);
            }
        } else if (protocol == "WireGuard") {
            child_pid = fork();
            if (child_pid == 0) {
                execlp("sudo", "sudo", "wg-quick", "up", cfg_path.c_str(), (char*) nullptr);
                std::cerr << "Failed to exec wg-quick!" << std::endl;
                std::exit(1);
            }
        }

        std::cout << "Connection established! Press Ctrl+C to disconnect." << std::endl;
        log(protocol + " connection established.");

        // Wait for Ctrl+C
        while (true) std::this_thread::sleep_for(std::chrono::seconds(1));

        // On disconnect
        if (protocol == "WireGuard") {
            std::string down_cmd = "sudo wg-quick down '" + cfg_path + "'";
            system(down_cmd.c_str());
            log("WireGuard disconnected.");
        }
    }
    return 0;
}

