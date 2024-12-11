// main.cpp

#include <SFML/Graphics.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <chrono>
#include <memory>
#include <cmath>

#include <algorithm>
#include "../colorFunctions/colorFunctions.h"

// -------------------------- Constantes --------------------------
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
const int GRID_ROWS = 4;
const int GRID_COLS = 4;
const int TOTAL_TRACKS = GRID_ROWS * GRID_COLS; // 16
const float SQUARE_WIDTH = WINDOW_WIDTH / static_cast<float>(GRID_COLS);
const float SQUARE_HEIGHT = WINDOW_HEIGHT / static_cast<float>(GRID_ROWS);
const float SHAPE_MAX_SCALE = 1.0f;
const float SHAPE_GROW_DURATION = 1.0f; // Segundos para alcanzar el tamaño máximo
const float SHAPE_LIFETIME = 2.0f;      // Segundos después de alcanzar el tamaño máximo
const float SHAPE_INITIAL_SCALE = 0.1f; // Escala inicial

// -------------------------- Estructuras --------------------------

// Estructura para representar un evento de crim2s
struct Crim2sEvent {
    int time; // en ticks
    int track;
    std::string msgType; // "note_on" o "note_off"
    int channel;
    int note;
    int velocity;
};

// Cola segura para hilos
std::queue<Crim2sEvent> eventQueue;
std::mutex queueMutex;
std::atomic<bool> running(true);

// Función para calcular el centroide de un polígono regular
sf::Vector2f calculateCentroid(const sf::ConvexShape& polygon) {
    sf::Vector2f centroid(0.f, 0.f);
    int pointCount = polygon.getPointCount();
    for (int i = 0; i < pointCount; ++i) {
        centroid += polygon.getPoint(i);
    }
    centroid.x /= pointCount;
    centroid.y /= pointCount;
    return centroid;
}

// Enum para definir tipos de formas
enum class ShapeType {
    Circle,       
    Oval,         
    Triangle,      
    Square,        
    Pentagon,      
    Hexagon,       
    Heptagon,      
    Octagon,       
    Nonagon,       
    Decagon,       
    Hendecagon,    
    Dodecagon,     
};

// Estructura para representar una forma dinámica
struct NoteShape {
    std::unique_ptr<sf::Shape> shape;
    float currentScale;
    float growthRate;      // Tasa de crecimiento por segundo
    float lifetime;        // Tiempo transcurrido desde que alcanzó el tamaño máximo
    bool active;

    // Constructor
    NoteShape(ShapeType type, sf::Color color, sf::Vector2f position, float growthRate) 
        : currentScale(SHAPE_INITIAL_SCALE), growthRate(growthRate), lifetime(0.0f), active(true) 
    {
        float radius = (std::min(SQUARE_WIDTH, SQUARE_HEIGHT) / 2.0f - 10.0f) / SHAPE_MAX_SCALE;

        switch (type) {
            case ShapeType::Circle: {
                auto circle = std::make_unique<sf::CircleShape>(radius);
                circle->setFillColor(color);
                circle->setOrigin(radius, radius);
                circle->setPosition(position);
                shape = std::move(circle);
                break;
            }
            case ShapeType::Oval: {
                auto oval = std::make_unique<sf::CircleShape>(radius);
                oval->setFillColor(color);
                oval->setOrigin(radius, radius);
                oval->setPosition(position);
                oval->setScale(1.0f, 0.6f); 
                shape = std::move(oval);
                break;
            }
            case ShapeType::Triangle: {
                auto triangle = std::make_unique<sf::ConvexShape>(3);
                for (int i = 0; i < 3; ++i) {
                    float angle = i * 2 * M_PI / 3 - M_PI / 2;
                    triangle->setPoint(i, sf::Vector2f(radius * std::cos(angle), radius * std::sin(angle)));
                }
                triangle->setFillColor(color);
                {
                    sf::Vector2f centroid = calculateCentroid(*triangle);
                    triangle->setOrigin(centroid);
                }
                triangle->setPosition(position);
                shape = std::move(triangle);
                break;
            }
            default: {
                int sides = 0;
                switch (type) {
                    case ShapeType::Square: sides = 4; break;
                    case ShapeType::Pentagon: sides = 5; break;
                    case ShapeType::Hexagon: sides = 6; break;
                    case ShapeType::Heptagon: sides = 7; break;
                    case ShapeType::Octagon: sides = 8; break;
                    case ShapeType::Nonagon: sides = 9; break;
                    case ShapeType::Decagon: sides = 10; break;
                    case ShapeType::Hendecagon: sides = 11; break;
                    case ShapeType::Dodecagon: sides = 12; break;
                    default: sides = 3; break;
                }

                auto polygon = std::make_unique<sf::ConvexShape>(sides);
                for (int i = 0; i < sides; ++i) {
                    float angle = i * 2 * M_PI / sides - M_PI / 2;
                    polygon->setPoint(i, sf::Vector2f(radius * std::cos(angle), radius * std::sin(angle)));
                }
                polygon->setFillColor(color);
                {
                    sf::Vector2f centroid = calculateCentroid(*polygon);
                    polygon->setOrigin(centroid);
                }
                polygon->setPosition(position);
                shape = std::move(polygon);
                break;
            }
        }

        // Establecer la escala inicial
        shape->setScale(currentScale, currentScale);
    }

    void update(float deltaTime) {
        if (currentScale < SHAPE_MAX_SCALE) {
            currentScale += growthRate * deltaTime;
            if (currentScale >= SHAPE_MAX_SCALE) {
                currentScale = SHAPE_MAX_SCALE;
            }
            shape->setScale(currentScale, currentScale);
        } else {
            lifetime += deltaTime;
            if (lifetime >= SHAPE_LIFETIME) {
                active = false;
            }
        }
    }

    bool isFinished() const {
        return !active;
    }
};

// Función para determinar el tipo de forma basado en la octava de la nota
ShapeType determineShapeType(int note) {
    int octave = (note / 12) - 1;
    switch (octave) {
        case 1:  return ShapeType::Circle;
        case 2:  return ShapeType::Oval;
        case 3:  return ShapeType::Triangle;
        case 4:  return ShapeType::Square;
        case 5:  return ShapeType::Pentagon;
        case 6:  return ShapeType::Hexagon;
        case 7:  return ShapeType::Heptagon;
        case 8:  return ShapeType::Octagon;
        case 9:  return ShapeType::Nonagon;
        case 10: return ShapeType::Decagon;
        case 11: return ShapeType::Hendecagon;
        case 12: return ShapeType::Dodecagon;
        default: return ShapeType::Circle; 
    }
}

// Función para leer el pipe y encolar eventos
// Función para leer el pipe y encolar eventos
void readCrim2sPipe(const std::string& pipePath) {
    std::ifstream pipe(pipePath);
    if (!pipe.is_open()) {
        std::cerr << "No se pudo abrir el pipe: " << pipePath << std::endl;
        running = false;
        return;
    }

    std::string line;
    // Leer eventos en tiempo real
    while (running && std::getline(pipe, line)) {
        if (line.empty()) continue;

        std::cout << "Mensaje recibido: " << line << std::endl;

        Crim2sEvent event;
        std::istringstream iss(line);
        std::string msg;
        if (line.find("Time=") == 0) {
            try {
                iss >> msg;
                event.time = std::stoi(msg.substr(5));
                iss >> msg;
                event.track = std::stoi(msg.substr(6));
                iss >> event.msgType;
                iss >> msg;
                event.channel = std::stoi(msg.substr(8));
                iss >> msg;
                event.note = std::stoi(msg.substr(5));
                iss >> msg;
                event.velocity = std::stoi(msg.substr(9));

                std::lock_guard<std::mutex> lock(queueMutex);
                eventQueue.push(event);
            } catch (const std::exception& e) {
                std::cerr << "Error al procesar mensaje: " << e.what() << std::endl;
            }
        }
    }
}


// Función para procesar eventos desde la cola
// Función para procesar eventos desde la cola
void processEvents(std::vector<std::vector<sf::CircleShape>>& trackShapes) {
    std::lock_guard<std::mutex> lock(queueMutex);
    while (!eventQueue.empty()) {
        Crim2sEvent crimEvent = eventQueue.front();
        eventQueue.pop();

        if (crimEvent.msgType == "note_on" && crimEvent.velocity > 0) {
            int row = crimEvent.track / GRID_COLS;
            int col = crimEvent.track % GRID_COLS;
            sf::CircleShape shape(20.0f);
            shape.setFillColor(setColorByOctave(crimEvent.note));
            shape.setPosition(col * SQUARE_WIDTH + SQUARE_WIDTH / 2, row * SQUARE_HEIGHT + SQUARE_HEIGHT / 2);
            trackShapes[crimEvent.track].push_back(shape);
        }
    }
}
int main(int argc, char* argv[]) {
    const std::string pipePath = "/tmp/midipipe";

    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Intérprete MIDI en Tiempo Real");
    window.setFramerateLimit(60);

    std::vector<std::vector<sf::CircleShape>> trackShapes(TOTAL_TRACKS);
    std::thread readerThread(readCrim2sPipe, pipePath);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        processEvents(trackShapes);

        window.clear(sf::Color::Black);

        for (int track = 0; track < TOTAL_TRACKS; ++track) {
            for (const auto& shape : trackShapes[track]) {
                window.draw(shape);
            }
        }

        window.display();
    }

    running = false;
    if (readerThread.joinable()) readerThread.join();

    return 0;
}