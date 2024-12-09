#include <SFML/Graphics.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <memory>
#include <mutex>
#include <rtmidi/RtMidi.h>
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

// Estructura para representar un evento de nota
struct NoteEvent {
    int note;
    int startTime;
    int endTime;
    bool noteOnSent = false;
    bool noteOffSent = false;
};

// Estructura para representar una pista
struct Track {
    std::vector<NoteEvent> notes;
};

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

// Lee el archivo .crim2s y devuelve las pistas
std::vector<Track> readCrim2sFile(const std::string& filename, int& ticksPerBeat) {
    std::ifstream file(filename);
    std::vector<Track> tracks;
    if (!file.is_open()) {
        std::cerr << "Error al abrir el archivo " << filename << std::endl;
        return tracks;
    }
    std::string line;
    int totalTracks = 0;
    while (std::getline(file, line)) {
        if (line.find("Ticks per beat:") != std::string::npos) {
            sscanf(line.c_str(), "Ticks per beat: %d", &ticksPerBeat);
        } else if (line.find("Número de pistas:") != std::string::npos) {
            sscanf(line.c_str(), "Número de pistas: %d", &totalTracks);
            tracks.resize(totalTracks);
        } else if (line.find("Eventos:") != std::string::npos) {
            break;
        }
    }

    while (std::getline(file, line)) {
        int time = 0, trackIndex = 0;
        char msgType[16];
        int channel = 0, note = 0, velocity = 0;
        if (sscanf(line.c_str(), "Time=%d Track=%d %s", &time, &trackIndex, msgType) >= 3) {
            if (std::string(msgType) == "note_on" || std::string(msgType) == "note_off") {
                if (sscanf(line.c_str(), "Time=%d Track=%d %s channel=%d note=%d velocity=%d",
                           &time, &trackIndex, msgType, &channel, &note, &velocity) == 6) {
                    if (trackIndex >= totalTracks) {
                        std::cerr << "Índice de pista inválido " << trackIndex << std::endl;
                        continue;
                    }
                    if (std::string(msgType) == "note_on" && velocity > 0) {
                        NoteEvent newNote;
                        newNote.note = note;
                        newNote.startTime = time;
                        newNote.endTime = -1;
                        tracks[trackIndex].notes.push_back(newNote);
                    } else {
                        for (auto& n : tracks[trackIndex].notes) {
                            if (n.note == note && n.endTime == -1) {
                                n.endTime = time;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    // Establecer endTime para notas que no lo tienen
    int maxTime = 0;
    for (const auto& track : tracks) {
        for (const auto& note : track.notes) {
            if (note.endTime > maxTime) {
                maxTime = note.endTime;
            }
        }
    }
    for (auto& track : tracks) {
        for (auto& note : track.notes) {
            if (note.endTime == -1) {
                note.endTime = maxTime;
            }
        }
    }

    file.close();
    return tracks;
}

// Determina el tipo de forma basado en la octava de la nota
ShapeType determineShapeType(int note) {
    int octave = (note / 12) - 1;
    switch (octave) {
        case 1: return ShapeType::Circle;
        case 2: return ShapeType::Oval;
        case 3: return ShapeType::Triangle;
        case 4: return ShapeType::Square;
        case 5: return ShapeType::Pentagon;
        case 6: return ShapeType::Hexagon;
        case 7: return ShapeType::Heptagon;
        case 8: return ShapeType::Octagon;
        case 9: return ShapeType::Nonagon;
        case 10:return ShapeType::Decagon;
        case 11:return ShapeType::Hendecagon;
        case 12:return ShapeType::Dodecagon;
        default:return ShapeType::Circle; 
    }
}

// Procesa una pista para crear formas basadas en las notas
void processTrack(Track& track, int trackIndex, float currentTimeSeconds, RtMidiOut& midiout, float pixelsPerSecond, float ticksPerSecond, std::vector<NoteShape>& shapes, float bpm) {
    for (auto& note : track.notes) {
        float noteStartTimeSeconds = note.startTime / ticksPerSecond;
        float noteEndTimeSeconds = note.endTime / ticksPerSecond;

        // Activar nota
        if (!note.noteOnSent && currentTimeSeconds >= noteStartTimeSeconds) {
            std::vector<unsigned char> message = { 0x90, static_cast<unsigned char>(note.note), 64 };
            midiout.sendMessage(&message);
            note.noteOnSent = true;

            ShapeType type = determineShapeType(note.note);

            int row = trackIndex / GRID_COLS;
            int col = trackIndex % GRID_COLS;
            float squareX = col * SQUARE_WIDTH;
            float squareY = row * SQUARE_HEIGHT;

            sf::Vector2f startPosition(squareX + SQUARE_WIDTH / 2.0f, squareY + SQUARE_HEIGHT / 2.0f);

            sf::Color noteColor = setColorByOctaveLinealAbss2(note.note);

            float growthRate = (SHAPE_MAX_SCALE - SHAPE_INITIAL_SCALE) / SHAPE_GROW_DURATION;
            shapes.emplace_back(type, noteColor, startPosition, growthRate);
        }

        // Desactivar nota
        if (!note.noteOffSent && currentTimeSeconds >= noteEndTimeSeconds) {
            std::vector<unsigned char> message = { 0x80, static_cast<unsigned char>(note.note), 64 };
            midiout.sendMessage(&message);
            note.noteOffSent = true;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Uso: " << argv[0] << " <ruta al archivo .crim2s> <bpm> <mix_strategy>" << std::endl;
        std::cerr << "mix_strategy: sum | average" << std::endl;
        return -1;
    }

    std::string crim2sFilePath = argv[1];
    float bpm = std::stof(argv[2]);
    std::string mixStrategy = argv[3];

    typedef sf::Color (*MixFunction)(const sf::Color&, const sf::Color&);
    MixFunction mixFunc = nullptr;

    if (mixStrategy == "sum") {
        mixFunc = mixColorsSum;
    } else if (mixStrategy == "average") {
        mixFunc = mixColorsAverage;
    } else {
        std::cerr << "Estrategia de mezcla desconocida: " << mixStrategy << std::endl;
        std::cerr << "Usa 'sum' o 'average'." << std::endl;
        return -1;
    }

    RtMidiOut midiout;
    unsigned int nPorts = midiout.getPortCount();
    if (nPorts == 0) {
        std::cout << "No hay puertos MIDI disponibles.\n";
        return -1;
    }
    midiout.openPort(0);

    int ticksPerBeat = 480; 
    std::vector<Track> tracks = readCrim2sFile(crim2sFilePath, ticksPerBeat);
    if (tracks.empty()) {
        std::cerr << "Error: no se encontraron pistas en el archivo." << std::endl;
        return -1;
    }

    if (tracks.size() < TOTAL_TRACKS) {
        tracks.resize(TOTAL_TRACKS);
    } else if (tracks.size() > TOTAL_TRACKS) {
        tracks.resize(TOTAL_TRACKS);
    }

    float beatsPerSecond = bpm / 60.0f;
    float ticksPerSecond = ticksPerBeat * beatsPerSecond;

    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Vista Transversal MIDI");
    window.setFramerateLimit(60);

    std::vector<std::vector<NoteShape>> trackShapes(TOTAL_TRACKS);

    sf::Clock totalClock;  
    sf::Clock deltaClock;  

    while (window.isOpen()) {
        float deltaTime = deltaClock.restart().asSeconds();

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        float currentTimeSeconds = totalClock.getElapsedTime().asSeconds();

        for (int i = 0; i < TOTAL_TRACKS; ++i) {
            processTrack(tracks[i], i, currentTimeSeconds, midiout, 0.0f, ticksPerSecond, trackShapes[i], bpm);
        }

        for (int i = 0; i < TOTAL_TRACKS; ++i) {
            auto& shapes = trackShapes[i];
            for (auto& shape : shapes) {
                shape.update(deltaTime);
            }
            shapes.erase(std::remove_if(shapes.begin(), shapes.end(),
                                        [](const NoteShape& s) { return s.isFinished(); }),
                         shapes.end());
        }

        window.clear(sf::Color(200, 200, 200));

        // Dibujar cuadrícula con mezcla de colores de las formas activas
        for (int row = 0; row < GRID_ROWS; ++row) {
            for (int col = 0; col < GRID_COLS; ++col) {
                int trackIndex = row * GRID_COLS + col;

                // Mezclar colores de las formas activas
                std::vector<sf::Color> activeColors;
                for (auto& shape : trackShapes[trackIndex]) {
                    activeColors.push_back(shape.shape->getFillColor());
                }

                sf::Color trackColor = sf::Color::Black;
                if (!activeColors.empty()) {
                    trackColor = applyMixingStrategy(activeColors, mixFunc);
                }

                sf::RectangleShape square(sf::Vector2f(SQUARE_WIDTH - 2, SQUARE_HEIGHT - 2));
                square.setFillColor(trackColor);
                square.setOutlineColor(sf::Color::Black);
                square.setOutlineThickness(1);
                square.setPosition(col * SQUARE_WIDTH + 1, row * SQUARE_HEIGHT + 1);
                window.draw(square);
            }
        }

        // Dibujar las formas encima de la cuadrícula
        for (int i = 0; i < TOTAL_TRACKS; ++i) {
            for (auto& shape : trackShapes[i]) {
                window.draw(*shape.shape);
            }
        }

        window.display();
    }

    return 0;
}
