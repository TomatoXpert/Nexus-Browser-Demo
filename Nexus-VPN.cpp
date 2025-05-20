import os
import sys
import subprocess
import logging
import time

try:
    import questionary
except ImportError:
    print("Installing required package 'questionary'...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "questionary"])
    import questionary

CONFIG_DIR = "./vpn_configs"

logging.basicConfig(
    filename="vpn_client.log",
    level=logging.INFO,
    format="%(asctime)s %(levelname)s: %(message)s"
)

def list_configs(protocol):
    configs = []
    if not os.path.exists(CONFIG_DIR):
        print(f"Config directory '{CONFIG_DIR}' does not exist. Please create it and add your VPN config files.")
        sys.exit(1)
    for f in os.listdir(CONFIG_DIR):
        if protocol == "OpenVPN" and f.lower().endswith(".ovpn"):
            configs.append(f)
        elif protocol == "WireGuard" and f.lower().endswith(".conf"):
            configs.append(f)
    return configs

def select_protocol():
    return questionary.select(
        "Choose VPN protocol:",
        choices=["OpenVPN", "WireGuard", "Exit"]
    ).ask()

def select_config(configs):
    choices = []
    for cfg in configs:
        if cfg.lower().endswith('.ovpn'):
            country = cfg.replace('openvpn_', '').replace('.ovpn', '').upper()
        else:
            country = cfg.replace('wg_', '').replace('.conf', '').upper()
        choices.append(f"{country} ({cfg})")
    answer = questionary.select(
        "Select country/server:",
        choices=choices
    ).ask()
    for cfg in configs:
        if cfg in answer:
            return cfg
    # fallback
    return configs[choices.index(answer)]

def start_openvpn(config_path):
    try:
        logging.info(f"Starting OpenVPN with config {config_path}")
        cmd = ["sudo", "openvpn", "--config", config_path]
        proc = subprocess.Popen(cmd)
        logging.info(f"OpenVPN process started (PID: {proc.pid})")
        return proc
    except Exception as e:
        logging.error(f"OpenVPN error: {e}")
        print(f"Error starting OpenVPN: {e}")
        return None

def start_wireguard(config_path):
    try:
        logging.info(f"Starting WireGuard with config {config_path}")
        cmd = ["sudo", "wg-quick", "up", config_path]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            logging.info("WireGuard started successfully")
            return True
        else:
            logging.error(f"WireGuard error: {result.stderr}")
            print(f"WireGuard error: {result.stderr}")
            return False
    except Exception as e:
        logging.error(f"WireGuard error: {e}")
        print(f"Error starting WireGuard: {e}")
        return False

def stop_openvpn(proc):
    try:
        proc.terminate()
        proc.wait(timeout=10)
        logging.info("OpenVPN terminated")
        print("OpenVPN disconnected.")
    except Exception as e:
        logging.error(f"OpenVPN termination error: {e}")
        print(f"Error terminating OpenVPN: {e}")

def stop_wireguard(config_path):
    try:
        cmd = ["sudo", "wg-quick", "down", config_path]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            logging.info("WireGuard terminated")
            print("WireGuard disconnected.")
        else:
            logging.error(f"WireGuard termination error: {result.stderr}")
            print(f"WireGuard termination error: {result.stderr}")
    except Exception as e:
        logging.error(f"WireGuard termination error: {e}")
        print(f"Error terminating WireGuard: {e}")

def main():
    print("=== Nexus VPN Client ===\n")
    current_proc = None
    current_protocol = None
    current_cfg = None

    while True:
        protocol = select_protocol()
        if protocol == "Exit":
            print("Goodbye!")
            break

        configs = list_configs(protocol)
        if not configs:
            print(f"No {protocol} configs found in {CONFIG_DIR}")
            continue

        cfg = select_config(configs)
        cfg_path = os.path.join(CONFIG_DIR, cfg)
        print(f"Connecting to {cfg}...")

        if protocol == "OpenVPN":
            current_proc = start_openvpn(cfg_path)
            current_protocol = "OpenVPN"
            current_cfg = None
            if not current_proc:
                continue
        elif protocol == "WireGuard":
            success = start_wireguard(cfg_path)
            current_proc = None
            current_protocol = "WireGuard"
            current_cfg = cfg_path
            if not success:
                continue

        print("Connection established! Press Ctrl+C to disconnect.")

        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            print("\nDisconnecting...")
            if current_protocol == "OpenVPN" and current_proc:
                stop_openvpn(current_proc)
            elif current_protocol == "WireGuard" and current_cfg:
                stop_wireguard(current_cfg)
            print("Disconnected.\n")

if __name__ == "__main__":
    main()
