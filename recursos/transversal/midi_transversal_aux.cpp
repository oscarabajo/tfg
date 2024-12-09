#include <SFML/Graphics.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <mutex>
#include <rtmidi/RtMidi.h>

struct NoteEvent {
    int note;
    int startTime;
    int endTime;
    bool noteOnSent = false;
    bool noteOffSent = false;
};

struct Track {
    std::vector<NoteEvent> notes;
};

std::mutex noteMutex;

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
    return sf::Color(colors[note % 12][0], colors[note % 12][1], colors[note % 12][2]);
}

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
        } else if (line.find("Número de pistas:") != std::string::npos) {
            sscanf(line.c_str(), "Número de pistas: %d", &totalTracks);
            tracks.resize(totalTracks);
        } else if (line.find("Eventos:") != std::string::npos) {
            break;
        }
    }

    // Leer los eventos
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

void processTrack(Track& track, int trackIndex, float currentTimeSeconds, float activationLineX, RtMidiOut &midiout, float pixelsPerSecond, float ticksPerSecond, sf::Color& trackColor) {
    for (auto& note : track.notes) {
        float noteStartTimeSeconds = note.startTime / ticksPerSecond;
        float noteEndTimeSeconds = note.endTime / ticksPerSecond;

        // Calcula la posición horizontal de la nota en la pantalla
        float xPosition = 800 - pixelsPerSecond * (currentTimeSeconds - noteStartTimeSeconds);
        float durationSeconds = noteEndTimeSeconds - noteStartTimeSeconds;
        float noteWidth = durationSeconds * pixelsPerSecond;

        // Enviar note_on cuando la nota cruce la línea de activación
        if (!note.noteOnSent && xPosition <= activationLineX) {
            std::vector<unsigned char> message = {0x90, static_cast<unsigned char>(note.note), 64};
            midiout.sendMessage(&message);
            note.noteOnSent = true;

            // Agregar el color de la nota al color de la pista
            sf::Color noteColor = setColorByOctaveLinealAbss2(note.note);
            trackColor.r = std::min(255, trackColor.r + noteColor.r);
            trackColor.g = std::min(255, trackColor.g + noteColor.g);
            trackColor.b = std::min(255, trackColor.b + noteColor.b);
        }

        // Enviar note_off cuando termine la duración de la nota
        if (!note.noteOffSent && xPosition + noteWidth <= activationLineX) {
            std::vector<unsigned char> message = {0x80, static_cast<unsigned char>(note.note), 64};
            midiout.sendMessage(&message);
            note.noteOffSent = true;

            // Restar el color de la nota del color de la pista
            sf::Color noteColor = setColorByOctaveLinealAbss2(note.note);
            trackColor.r = std::max(0, trackColor.r - noteColor.r);
            trackColor.g = std::max(0, trackColor.g - noteColor.g);
            trackColor.b = std::max(0, trackColor.b - noteColor.b);
        }
    }
}

int main(int argc, char* argv[]) {
    // Verifica que el usuario proporcione el archivo de entrada y los BPM como argumentos
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <ruta al archivo .crim2s> <bpm>" << std::endl;
        return -1;
    }
    std::string crim2sFilePath = argv[1];
    float bpm = std::stof(argv[2]);

    // Inicializa salida MIDI
    RtMidiOut midiout;
    unsigned int nPorts = midiout.getPortCount();
    if (nPorts == 0) {
        std::cout << "No hay puertos MIDI disponibles.\n";
        return -1;
    }
    midiout.openPort(0);

    int ticksPerBeat = 480; // Valor por defecto, se actualizará al leer el archivo
    std::vector<Track> tracks = readCrim2sFile(crim2sFilePath, ticksPerBeat);
    if (tracks.empty()) {
        std::cerr << "Error: no se encontraron pistas en el archivo." << std::endl;
        return -1;
    }

    // Calcula ticks por segundo
    float beatsPerSecond = bpm / 60.0f;
    float ticksPerSecond = ticksPerBeat * beatsPerSecond;

    // Configura ventana de visualización
    // Ajustamos la altura para mostrar solo la vista transversal
    sf::RenderWindow window(sf::VideoMode(800, 200), "Vista Transversal MIDI");

    // Define la altura de cada pista y de cada nota
    int numTracks = tracks.size();
    // La altura total es 200, reservamos espacio para los cuadrados
    float trackHeight = 150.0f / numTracks; // Ajuste para ajustar la vista transversal
    float pixelsPerSecond = 100.0f; // Este valor ya no afecta la visualización principal

    sf::Clock clock;
    float activationLineX = 200.0f; // Este ya no se utiliza visualmente
    // Eliminamos el vector noteShapes ya que no se dibujan

    // Variables para la vista transversal
    std::vector<sf::Color> trackColors(numTracks, sf::Color::Black);

    // Bucle principal de la ventana
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        sf::Time elapsed = clock.getElapsedTime();
        float currentTimeSeconds = elapsed.asSeconds();

        window.clear();

        // Procesa cada pista para actualización de colores y reproducción MIDI
        for (int i = 0; i < numTracks; ++i) {
            processTrack(tracks[i], i, currentTimeSeconds, activationLineX, midiout, pixelsPerSecond, ticksPerSecond, trackColors[i]);
        }

        // Dibuja la vista transversal en la ventana
        float squareWidth = 800.0f / numTracks;
        for (int i = 0; i < numTracks; ++i) {
            sf::RectangleShape square(sf::Vector2f(squareWidth - 10, 150)); // Altura de 150 píxeles
            square.setFillColor(trackColors[i]);
            square.setPosition(i * squareWidth + 5, 25); // Centrado verticalmente en 200px de altura
            window.draw(square);
        }

        window.display(); // Muestra el contenido en la ventana
    }

    return 0;
}