#include <SFML/Graphics.hpp>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <iostream>
#include <cmath>
#include <memory>
#include <rtmidi/RtMidi.h>
#include <algorithm>
#include <sstream>
#include <unordered_map>

// -------------------------- Constantes --------------------------
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;
const int TOTAL_TRACKS = 16; // Número total de pistas
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

// Estructura para representar una forma dinámica asociada a un nodo
struct NoteShape {
    std::shared_ptr<sf::Shape> shape;
    float currentScale;
    float growthRate;      // Tasa de crecimiento por segundo
    float lifetime;        // Tiempo transcurrido desde que alcanzó el tamaño máximo
    bool active;

    // Constructor para un cuadrado
    NoteShape(sf::Color color, sf::Vector2f position, float growthRateParam) 
        : currentScale(SHAPE_INITIAL_SCALE), growthRate(growthRateParam), lifetime(0.0f), active(true) 
    {
        float size = 40.0f; // Tamaño del cuadrado
        auto square = std::make_shared<sf::RectangleShape>(sf::Vector2f(size, size));
        square->setFillColor(color);
        square->setOrigin(size / 2.0f, size / 2.0f); // Centrar el origen
        square->setPosition(position);
        shape = square;
        // Establecer la escala inicial
        shape->setScale(currentScale, currentScale);
    }

    // Eliminar constructor de copia y operador de asignación de copia
    NoteShape(const NoteShape&) = delete;
    NoteShape& operator=(const NoteShape&) = delete;

    // Habilitar constructor de movimiento y operador de asignación de movimiento
    NoteShape(NoteShape&&) = default;
    NoteShape& operator=(NoteShape&&) = default;

    // Actualizar el estado de la forma
    void update(float deltaTime) {
        if (currentScale < SHAPE_MAX_SCALE) {
            currentScale += growthRate * deltaTime;
            if (currentScale >= SHAPE_MAX_SCALE) {
                currentScale = SHAPE_MAX_SCALE;
            }
            shape->setScale(currentScale, currentScale);
        }
        else {
            lifetime += deltaTime;
            // Desactivar la forma después de su tiempo de vida
            if (lifetime >= SHAPE_LIFETIME) {
                active = false;
            }
        }
    }

    // Verificar si la forma ha terminado su ciclo
    bool isFinished() const {
        return !active;
    }
};

// Estructura para representar un nodo en el árbol binario
struct TreeNode {
    int trackIndex; // Índice de la pista asociada
    std::shared_ptr<TreeNode> left;
    std::shared_ptr<TreeNode> right;
    sf::Vector2f position;
    std::shared_ptr<NoteShape> noteShape;

    TreeNode(int index, sf::Vector2f pos, std::shared_ptr<NoteShape> shape)
        : trackIndex(index), position(pos), noteShape(shape), left(nullptr), right(nullptr) {}
};

// -------------------------- Funciones Auxiliares --------------------------

// Asigna un color basado en la octava de la nota
sf::Color setColorByOctaveLinealAbss2(int note) {
    float colors[12][3] = {
        {255, 0, 0},     // C
        {255, 127, 0},   // C#
        {255, 255, 0},   // D
        {127, 255, 0},   // D#
        {0, 255, 0},     // E
        {0, 255, 127},   // F
        {0, 255, 255},   // F#
        {0, 127, 255},   // G
        {0, 0, 255},     // G#
        {127, 0, 255},   // A
        {255, 0, 255},   // A#
        {255, 0, 127}    // B
    };
    return sf::Color(static_cast<sf::Uint8>(colors[note % 12][0]),
                     static_cast<sf::Uint8>(colors[note % 12][1]),
                     static_cast<sf::Uint8>(colors[note % 12][2]));
}

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
    // Leer información del encabezado
    while (std::getline(file, line)) {
        if (line.find("Ticks per beat:") != std::string::npos) {
            sscanf(line.c_str(), "Ticks per beat: %d", &ticksPerBeat);
        }
        else if (line.find("Número de pistas:") != std::string::npos) {
            sscanf(line.c_str(), "Número de pistas: %d", &totalTracks);
            tracks.resize(totalTracks);
        }
        else if (line.find("Eventos:") != std::string::npos) {
            break;
        }
    }

    // Leer los eventos
    while (std::getline(file, line)) {
        int time = 0, trackIndex = 0;
        char msgType[16];
        int channel = 0, note = 0, velocity = 0;
        // Utilizar stringstream para mayor flexibilidad
        std::stringstream ss(line);
        std::string token;
        ss >> token; // Time=...
        if (token.find("Time=") == 0) {
            time = std::stoi(token.substr(5));
        }
        ss >> token; // Track=...
        if (token.find("Track=") == 0) {
            trackIndex = std::stoi(token.substr(6));
        }
        ss >> token; // msgType
        strcpy(msgType, token.c_str());
        if (std::string(msgType) == "note_on" || std::string(msgType) == "note_off") {
            ss >> token; // channel=...
            if (token.find("channel=") == 0) {
                channel = std::stoi(token.substr(8));
            }
            ss >> token; // note=...
            if (token.find("note=") == 0) {
                note = std::stoi(token.substr(5));
            }
            ss >> token; // velocity=...
            if (token.find("velocity=") == 0) {
                velocity = std::stoi(token.substr(9));
            }

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
            }
            else {
                // Encontrar la nota y establecer endTime
                for (auto& n : tracks[trackIndex].notes) {
                    if (n.note == note && n.endTime == -1) {
                        n.endTime = time;
                        break;
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

// Procesa una pista para crear formas basadas en las notas
void processTrack(Track& track, int trackIndex, float currentTimeSeconds, RtMidiOut& midiout, float ticksPerSecond, std::vector<std::shared_ptr<NoteShape>>& shapes) {
    for (auto& note : track.notes) {
        float noteStartTimeSeconds = note.startTime / ticksPerSecond;
        float noteEndTimeSeconds = note.endTime / ticksPerSecond;

        // Activar nota
        if (!note.noteOnSent && currentTimeSeconds >= noteStartTimeSeconds) {
            // Enviar mensaje MIDI note_on
            std::vector<unsigned char> message = { 0x90, static_cast<unsigned char>(note.note), 64 };
            midiout.sendMessage(&message);
            note.noteOnSent = true;

            // Obtener el color de la nota
            sf::Color noteColor = setColorByOctaveLinealAbss2(note.note);

            // Calcular la posición del nodo en el árbol binario
            // Suponiendo que trackIndex determina el nivel del árbol
            int level = std::floor(std::log2(trackIndex + 1));
            int positionInLevel = trackIndex - std::pow(2, level) + 1;

            float horizontalSpacing = WINDOW_WIDTH / std::pow(2, level + 1);
            float xPos = horizontalSpacing + positionInLevel * horizontalSpacing * 2;
            float yPos = 100.0f + level * 100.0f;

            sf::Vector2f nodePosition(xPos, yPos);

            // Tasa de crecimiento para alcanzar el tamaño máximo en SHAPE_GROW_DURATION segundos
            float growthRate = (SHAPE_MAX_SCALE - SHAPE_INITIAL_SCALE) / SHAPE_GROW_DURATION;

            // Crear la forma y añadirla al vector de formas
            shapes.emplace_back(std::make_shared<NoteShape>(noteColor, nodePosition, growthRate));
        }

        // Desactivar nota
        if (!note.noteOffSent && currentTimeSeconds >= noteEndTimeSeconds) {
            // Enviar mensaje MIDI note_off
            std::vector<unsigned char> message = { 0x80, static_cast<unsigned char>(note.note), 64 };
            midiout.sendMessage(&message);
            note.noteOffSent = true;

            // Remover la forma correspondiente
            if (!shapes.empty()) {
                shapes.pop_back();
            }
        }
    }
}

// Construye el árbol binario basado en las pistas activas
std::shared_ptr<TreeNode> buildBinaryTree(int currentTrack, const std::unordered_map<int, std::vector<int>>& trackHierarchy, const std::unordered_map<int, std::shared_ptr<NoteShape>>& shapeMap) {
    // Verificar si la pista actual tiene una forma activa
    if (shapeMap.find(currentTrack) == shapeMap.end())
        return nullptr;

    // Obtener la forma correspondiente
    auto it = shapeMap.find(currentTrack);
    std::shared_ptr<NoteShape> nodeShape = it->second;

    sf::Vector2f position = nodeShape->shape->getPosition();

    auto node = std::make_shared<TreeNode>(currentTrack, position, nodeShape);

    // Obtener hijos (izquierdo y derecho)
    if (trackHierarchy.find(currentTrack) != trackHierarchy.end()) {
        const std::vector<int>& children = trackHierarchy.at(currentTrack);
        if (children.size() >= 1) {
            node->left = buildBinaryTree(children[0], trackHierarchy, shapeMap);
        }
        if (children.size() >= 2) {
            node->right = buildBinaryTree(children[1], trackHierarchy, shapeMap);
        }
    }

    return node;
}

// Dibuja el árbol binario en la ventana
void drawTree(sf::RenderWindow& window, std::shared_ptr<TreeNode> node) {
    if (!node)
        return;

    // Dibujar líneas a los hijos
    if (node->left) {
        sf::Vertex line[] =
        {
            sf::Vertex(node->position, sf::Color::White),
            sf::Vertex(node->left->position, sf::Color::White)
        };
        window.draw(line, 2, sf::Lines);
        drawTree(window, node->left);
    }
    if (node->right) {
        sf::Vertex line[] =
        {
            sf::Vertex(node->position, sf::Color::White),
            sf::Vertex(node->right->position, sf::Color::White)
        };
        window.draw(line, 2, sf::Lines);
        drawTree(window, node->right);
    }

    // Dibujar el nodo
    if (node->noteShape && node->noteShape->active) {
        window.draw(*node->noteShape->shape);
    }
}

int main(int argc, char* argv[]) {
    // Verificar argumentos de línea de comandos
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <ruta al archivo .crim2s> <bpm>" << std::endl;
        return -1;
    }
    std::string crim2sFilePath = argv[1];
    float bpm = std::stof(argv[2]);

    // Inicializar salida MIDI
    RtMidiOut midiout;
    unsigned int nPorts = midiout.getPortCount();
    if (nPorts == 0) {
        std::cout << "No hay puertos MIDI disponibles.\n";
        return -1;
    }
    midiout.openPort(0);
    std::cout << "Conexión MIDI establecida correctamente entre RtMidi y FluidSynth.\n";

    // Leer el archivo .crim2s
    int ticksPerBeat = 480; // Valor por defecto, se actualizará al leer el archivo
    std::vector<Track> tracks = readCrim2sFile(crim2sFilePath, ticksPerBeat);
    if (tracks.empty()) {
        std::cerr << "Error: no se encontraron pistas en el archivo.\n";
        return -1;
    }
    std::cout << "[*] Extracción MIDI completada.\n";

    // Calcular ticks por segundo
    float beatsPerSecond = bpm / 60.0f;
    float ticksPerSecond = ticksPerBeat * beatsPerSecond;

    // Configurar ventana de visualización
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Visualización de Árbol Binario MIDI (Solo Cuadrados)");
    window.setFramerateLimit(60); // Limitar a 60 FPS

    // Mapear jerarquía de pistas (para árbol binario)
    // Suponiendo que Track 0 es la raíz, Track 1 y 2 son sus hijos, Track 3 y 4 son hijos de Track 1, etc.
    std::unordered_map<int, std::vector<int>> trackHierarchy;
    for (int i = 0; i < TOTAL_TRACKS; ++i) {
        int leftChild = 2 * i + 1;
        int rightChild = 2 * i + 2;
        std::vector<int> children;
        if (leftChild < TOTAL_TRACKS)
            children.push_back(leftChild);
        if (rightChild < TOTAL_TRACKS)
            children.push_back(rightChild);
        if (!children.empty())
            trackHierarchy[i] = children;
    }

    // Variables para gestionar las formas activas por pista
    std::unordered_map<int, std::vector<std::shared_ptr<NoteShape>>> activeShapesMap; // trackIndex -> formas activas

    // Reloj para el tiempo total y deltaTime
    sf::Clock totalClock;  // Tiempo total transcurrido
    sf::Clock deltaClock;  // DeltaTime entre frames

    std::cout << "[*] Inicio de la visualización.\n";

    // Bucle principal
    while (window.isOpen()) {
        // Calcular deltaTime
        float deltaTime = deltaClock.restart().asSeconds();

        // Manejar eventos
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Calcular tiempo actual en segundos
        float currentTimeSeconds = totalClock.getElapsedTime().asSeconds();

        // Actualizar cada pista
        for (int i = 0; i < TOTAL_TRACKS; ++i) {
            processTrack(tracks[i], i, currentTimeSeconds, midiout, ticksPerSecond, activeShapesMap[i]);
        }

        // Actualizar formas
        for (int i = 0; i < TOTAL_TRACKS; ++i) {
            auto& shapes = activeShapesMap[i];
            for (auto& shape : shapes) {
                shape->update(deltaTime);
            }
            // Eliminar formas que han terminado su ciclo
            shapes.erase(std::remove_if(shapes.begin(), shapes.end(),
                [](const std::shared_ptr<NoteShape>& s) { return s->isFinished(); }), shapes.end());
        }

        // Limpiar ventana con fondo negro
        window.clear(sf::Color::Black);

        // Construir el árbol binario basado en las pistas activas
        // Comenzamos desde la raíz (Track 0)
        std::unordered_map<int, std::shared_ptr<NoteShape>> shapeMap;
        for (int i = 0; i < TOTAL_TRACKS; ++i) {
            if (!activeShapesMap[i].empty()) {
                // Tomamos la última forma activa para representar la pista
                shapeMap[i] = activeShapesMap[i].back();
            }
        }

        auto treeRoot = buildBinaryTree(0, trackHierarchy, shapeMap);

        // Dibujar el árbol
        drawTree(window, treeRoot);

        // Mostrar todo en la ventana
        window.display();
    }

    std::cout << "Cerrando FluidSynth...\n";
    // RtMidiOut se cerrará automáticamente al destructurarse
    std::cout << "Proceso completado.\n";

    return 0;
}
