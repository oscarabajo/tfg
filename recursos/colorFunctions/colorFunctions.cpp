// colorFunctions.cpp

#include "colorFunctions.h"
#include <algorithm>
#include <cstdio>

// Asigna un color a la nota basado en la octava y la escala de colores
sf::Color setColorByOctave(int note) {
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
    // Asignar un alpha de 255 para colores opacos
    return sf::Color(
        static_cast<sf::Uint8>(colors[note % 12][0]),
        static_cast<sf::Uint8>(colors[note % 12][1]),
        static_cast<sf::Uint8>(colors[note % 12][2]),
        255 // Opaco
    );
}

// Función auxiliar para mezclar dos colores sumando sus componentes RGB
sf::Color mixColorsSum(const sf::Color& a, const sf::Color& b) {
    int sumR = static_cast<int>(a.r) + static_cast<int>(b.r);
    int sumG = static_cast<int>(a.g) + static_cast<int>(b.g);
    int sumB = static_cast<int>(a.b) + static_cast<int>(b.b);
    int sumA = static_cast<int>(a.a) + static_cast<int>(b.a);
    
    sf::Uint8 r = static_cast<sf::Uint8>(std::min(sumR, 255));
    sf::Uint8 g = static_cast<sf::Uint8>(std::min(sumG, 255));
    sf::Uint8 b_ = static_cast<sf::Uint8>(std::min(sumB, 255));
    sf::Uint8 alpha = static_cast<sf::Uint8>(std::min(sumA, 255));
    
    return sf::Color(r, g, b_, alpha); // Mezcla también el alpha
}

// Función auxiliar para mezclar dos colores promediando sus componentes RGB
sf::Color mixColorsAverage(const sf::Color& a, const sf::Color& b) {
    int avgR = (static_cast<int>(a.r) + static_cast<int>(b.r)) / 2;
    int avgG = (static_cast<int>(a.g) + static_cast<int>(b.g)) / 2;
    int avgB = (static_cast<int>(a.b) + static_cast<int>(b.b)) / 2;
    int avgA = (static_cast<int>(a.a) + static_cast<int>(b.a)) / 2;
    
    sf::Uint8 r = static_cast<sf::Uint8>(std::min(avgR, 255));
    sf::Uint8 g = static_cast<sf::Uint8>(std::min(avgG, 255));
    sf::Uint8 b_ = static_cast<sf::Uint8>(std::min(avgB, 255));
    sf::Uint8 alpha = static_cast<sf::Uint8>(std::min(avgA, 255));
    
    printf("[*] MEDIA\n");
    return sf::Color(r, g, b_, alpha);
}

// Implementación de applyMixingStrategy
sf::Color applyMixingStrategy(const std::vector<sf::Color>& colors, sf::Color (*mixFunc)(const sf::Color&, const sf::Color&)) {
    if (colors.empty()) {
        return sf::Color::Black;
    }
    sf::Color mixed = colors[0];
    for (size_t i = 1; i < colors.size(); ++i) {
        mixed = mixFunc(mixed, colors[i]);
    }
    return mixed;
    
}
sf::Color setColorByOctaveBlue(int note) {
    // Definir una gama cromática de azules para las 12 notas
    sf::Color colors[12] = {
        sf::Color(173, 216, 230), // C - Light Blue
        sf::Color(135, 206, 235), // C# - Sky Blue
        sf::Color(70, 130, 180),  // D - Steel Blue
        sf::Color(100, 149, 237), // D# - Cornflower Blue
        sf::Color(65, 105, 225),  // E - Royal Blue
        sf::Color(0, 0, 255),     // F - Blue
        sf::Color(25, 25, 112),   // F# - Midnight Blue
        sf::Color(30, 144, 255),  // G - Dodger Blue
        sf::Color(0, 191, 255),    // G# - Deep Sky Blue
        sf::Color(135, 206, 250), // A - Light Sky Blue
        sf::Color(70, 130, 180),  // A# - Steel Blue
        sf::Color(0, 0, 139)       // B - Dark Blue
    };
    
    // Asignar un alpha de 255 para colores opacos
    return sf::Color(
        colors[note % 12].r,
        colors[note % 12].g,
        colors[note % 12].b,
        255 // Opaco
    );
}
