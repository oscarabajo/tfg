#include <SFML/Graphics.hpp>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <cmath>
#include "../colorFunctions/colorFunctions.h"

// -------------------------- Constantes --------------------------
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;

// -------------------------- Estructuras --------------------------

struct Crim2sEvent {
    int time;        // en ticks
    int track;       // pista, aunque solo usamos una
    std::string msgType; // "note_on" o "note_off"
    int note;        // Nota MIDI
    int velocity;    // Velocidad MIDI
    int extraTime;   // Campo adicional opcional (por ejemplo, time=0)
    int channel;
};

// Cola segura para hilos
std::queue<Crim2sEvent> eventQueue;
std::mutex queueMutex;
std::atomic<bool> running(true);

// -------------------------- Funciones --------------------------
void readCrim2sPipe(const std::string& pipePath) {
    std::ifstream pipe(pipePath);
    if (!pipe.is_open()) {
        std::cerr << "No se pudo abrir el pipe: " << pipePath << std::endl;
        running = false;
        return;
    }

    std::string line;
    while (running && std::getline(pipe, line)) {
        if (line.empty()) continue;

        std::cout << "Línea recibida: " << line << std::endl; // Depuración

        Crim2sEvent event;
        std::istringstream iss(line);
        std::string msg;

        try {
            // Validar y procesar cada componente
            if (!(iss >> msg) || msg.substr(0, 5) != "Time=") {
                throw std::invalid_argument("Formato inválido para Time");
            }
            event.time = std::stoi(msg.substr(5));

            if (!(iss >> msg) || msg.substr(0, 6) != "Track=") {
                throw std::invalid_argument("Formato inválido para Track");
            }
            event.track = std::stoi(msg.substr(6));

            if (!(iss >> event.msgType) || (event.msgType != "note_on" && event.msgType != "note_off")) {
                throw std::invalid_argument("Formato inválido para MsgType");
            }
            if (!(iss >> msg) || msg.substr(0, 8) != "channel=") {
                    throw std::invalid_argument("Formato inválido para Channel");
                }
                event.channel = std::stoi(msg.substr(8));

            if (!(iss >> msg) || msg.substr(0, 5) != "note=") {
                throw std::invalid_argument("Formato inválido para Note");
            }
            event.note = std::stoi(msg.substr(5));

            if (!(iss >> msg) || msg.substr(0, 9) != "velocity=") {
                throw std::invalid_argument("Formato inválido para Velocity");
            }
            event.velocity = std::stoi(msg.substr(9));
            
            if (!(iss >> msg) || msg.substr(0, 5) != "time=") {
                throw std::invalid_argument("Formato inválido para time");
            }
            event.extraTime = std::stoi(msg.substr(5));


            // Agregar evento a la cola
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                eventQueue.push(event);
            }

            std::cout << "Evento procesado: Time=" << event.time
                      << ", Track=" << event.track
                      << ", MsgType=" << event.msgType
                      << ", Note=" << event.note
                      << ", Velocity=" << event.velocity
                      << ", ExtraTime=" << event.extraTime << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Error al procesar mensaje: " << e.what() << " en línea: " << line << std::endl;
        }
    }
}



// Función para procesar eventos y mezclar colores
sf::Color processEvents(sf::Color currentColor) {
    std::vector<sf::Color> colorsToMix;

    std::lock_guard<std::mutex> lock(queueMutex);
    while (!eventQueue.empty()) {
        Crim2sEvent crimEvent = eventQueue.front();
        eventQueue.pop();

        if (crimEvent.msgType == "note_on" && crimEvent.velocity > 0) {
            colorsToMix.push_back(setColorByOctave(crimEvent.note));
        }
    }

    if (!colorsToMix.empty()) {
        return applyMixingStrategy(colorsToMix, mixColorsAverage); // Usa la estrategia deseada
    }

    return currentColor;
}

int main(int argc, char* argv[]) {
    const std::string pipePath = "/tmp/midipipe";

    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Intérprete MIDI en Tiempo Real");
    window.setFramerateLimit(60);

    // Círculo que ocupa toda la ventana
    sf::CircleShape mainCircle(std::min(WINDOW_WIDTH, WINDOW_HEIGHT) / 2.0f);
    mainCircle.setOrigin(mainCircle.getRadius(), mainCircle.getRadius());
    mainCircle.setPosition(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
    mainCircle.setFillColor(sf::Color::Black);

    // Color inicial
    sf::Color currentColor = sf::Color::Black;

    // Hilo para leer el pipe
    std::thread readerThread(readCrim2sPipe, pipePath);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Procesar eventos y actualizar el color
        currentColor = processEvents(currentColor);
        mainCircle.setFillColor(currentColor);

        // Dibujar
        window.clear();
        window.draw(mainCircle);
        window.display();
    }

    running = false;
    if (readerThread.joinable()) readerThread.join();

    return 0;
}
