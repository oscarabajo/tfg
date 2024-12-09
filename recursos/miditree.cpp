#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>

// -------------------------- Constantes --------------------------
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
const int MAX_TRACKS = 5; // Número máximo de pistas (niveles)
const float NODE_RADIUS = 20.0f;
const float VERTICAL_SPACING = 100.0f; // Espaciado vertical entre niveles
const float HORIZONTAL_SPACING = 400.0f; // Espaciado horizontal inicial

// -------------------------- Estructuras --------------------------

// Estructura para representar un evento de nota
struct NoteEvent {
    int note;
    int startTime;
    int endTime;
};

// Estructura para representar una pista
struct Track {
    std::vector<NoteEvent> notes;
};

// Enum para definir tipos de formas
enum class ShapeType {
    Circle,
    Triangle,
    Square,
    // Agrega más tipos de formas según tus necesidades
};

// -------------------------- Funciones Auxiliares --------------------------

// Determina el tipo de forma basado en la nota
ShapeType determineShapeType(int note) {
    switch (note % 3) {
        case 0: return ShapeType::Circle;
        case 1: return ShapeType::Triangle;
        case 2: return ShapeType::Square;
        default: return ShapeType::Circle;
    }
}

// Crea una forma basada en el tipo y la posición
sf::Shape* createShape(ShapeType type, const sf::Vector2f& position, sf::Color color) {
    switch (type) {
        case ShapeType::Circle: {
            auto* shape = new sf::CircleShape(NODE_RADIUS);
            shape->setFillColor(color);
            shape->setOrigin(NODE_RADIUS, NODE_RADIUS);
            shape->setPosition(position);
            return shape;
        }
        case ShapeType::Triangle: {
            auto* shape = new sf::ConvexShape(3);
            shape->setPoint(0, sf::Vector2f(0, -NODE_RADIUS));
            shape->setPoint(1, sf::Vector2f(-NODE_RADIUS, NODE_RADIUS));
            shape->setPoint(2, sf::Vector2f(NODE_RADIUS, NODE_RADIUS));
            shape->setFillColor(color);
            shape->setOrigin(0, 0);
            shape->setPosition(position);
            return shape;
        }
        case ShapeType::Square: {
            auto* shape = new sf::RectangleShape(sf::Vector2f(2 * NODE_RADIUS, 2 * NODE_RADIUS));
            shape->setFillColor(color);
            shape->setOrigin(NODE_RADIUS, NODE_RADIUS);
            shape->setPosition(position);
            return shape;
        }
        default:
            return nullptr;
    }
}

// Lee el archivo .crim2s y devuelve las pistas
std::vector<Track> readCrim2sFile(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<Track> tracks;
    if (!file.is_open()) {
        std::cerr << "Error al abrir el archivo: " << filename << std::endl;
        return tracks;
    }

    std::string line;
    int totalTracks = 0;

    while (std::getline(file, line)) {
        if (line.find("Número de pistas:") != std::string::npos) {
            sscanf(line.c_str(), "Número de pistas: %d", &totalTracks);
            tracks.resize(totalTracks);
        } else if (line.find("Eventos:") != std::string::npos) {
            break;
        }
    }

    while (std::getline(file, line)) {
        int trackIndex = 0, note = 0;
        if (sscanf(line.c_str(), "Track=%d note=%d", &trackIndex, &note) == 2) {
            if (trackIndex >= totalTracks) continue;
            NoteEvent event;
            event.note = note;
            tracks[trackIndex].notes.push_back(event);
        }
    }

    return tracks;
}

// Genera el árbol visual
void generateTree(const std::vector<Track>& tracks, sf::RenderWindow& window) {
    std::vector<sf::Shape*> shapes;

    for (int level = 0; level < tracks.size() && level < MAX_TRACKS; ++level) {
        const auto& track = tracks[level];
        int nodesInLevel = track.notes.size();
        float levelY = (level + 1) * VERTICAL_SPACING;
        float spacing = HORIZONTAL_SPACING / std::max(1, nodesInLevel);

        for (int node = 0; node < nodesInLevel; ++node) {
            float nodeX = (node + 1) * spacing;
            int note = track.notes[node].note;

            ShapeType type = determineShapeType(note);
            sf::Color color = sf::Color(100 + (note % 155), 100, 255 - (note % 155));

            auto* shape = createShape(type, sf::Vector2f(nodeX, levelY), color);
            if (shape) shapes.push_back(shape);
        }
    }

    // Dibujar en la ventana
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        window.clear(sf::Color::Black);
        for (auto* shape : shapes) {
            window.draw(*shape);
        }
        window.display();
    }

    // Limpiar memoria
    for (auto* shape : shapes) {
        delete shape;
    }
}

// -------------------------- Función Principal --------------------------

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <ruta al archivo .crim2s>" << std::endl;
        return -1;
    }

    std::string crim2sFilePath = argv[1];

    auto tracks = readCrim2sFile(crim2sFilePath);
    if (tracks.empty()) {
        std::cerr << "Error: No se pudieron leer las pistas del archivo." << std::endl;
        return -1;
    }

    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Árbol de Notas MIDI");

    generateTree(tracks, window);

    return 0;
}
