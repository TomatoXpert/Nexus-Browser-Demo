#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <cmath>

// Load JSON
nlohmann::json load_servers(const std::string& file) {
    std::ifstream in(file);
    return nlohmann::json::parse(in);
}

// Convert lat/lon to x/y on map (Mercator projection approximation)
sf::Vector2f geo_to_map(float lat, float lon, sf::Vector2u size) {
    float x = (lon + 180.f) * (size.x / 360.f);
    float y = (90.f - lat) * (size.y / 180.f);
    return {x, y};
}

int main() {
    sf::RenderWindow window(sf::VideoMode(1024, 512), "Nexus VPN Map");
    sf::Texture mapTexture;
    mapTexture.loadFromFile("world_map.jpg");
    sf::Sprite map(mapTexture);

    auto servers = load_servers("servers.json");
    sf::Font font;
    font.loadFromFile("arial.ttf");

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event))
            if (event.type == sf::Event::Closed)
                window.close();

        window.clear();
        window.draw(map);

        for (auto& server : servers) {
            auto pos = geo_to_map(server["latitude"], server["longitude"], mapTexture.getSize());
            sf::CircleShape dot(6);
            dot.setPosition(pos);
            dot.setFillColor(sf::Color::Green);
            window.draw(dot);

            // Hover detection
            if (dot.getGlobalBounds().contains(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y)) {
                sf::Text label(server["name"], font, 16);
                label.setPosition(pos.x + 10, pos.y);
                label.setFillColor(sf::Color::White);
                window.draw(label);

                // If clicked
                if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                    std::string cfg = server["config"];
                    std::string proto = server["protocol"];
                    std::string log = "Connecting to " + server["name"].get<std::string>();
                    std::system(("echo " + log + " >> vpn_client.log").c_str());

                    std::string cmd;
                    if (proto == "OpenVPN")
                        cmd = "sudo openvpn --config vpn_configs/" + cfg;
                    else
                        cmd = "sudo wg-quick up vpn_configs/" + cfg;

                    std::system(cmd.c_str());
                }
            }
        }

        window.display();
    }
    return 0;
}
