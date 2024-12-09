#include "colorFunctions.h"
#include <algorithm>

// Asigna un color a la nota basado en la octava y la escala de colores
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

// Función auxiliar para mezclar dos colores sumando sus componentes RGB
sf::Color mixColorsSum(const sf::Color& a, const sf::Color& b) {
    sf::Uint8 r = std::min(static_cast<int>(a.r) + static_cast<int>(b.r), 255);
    sf::Uint8 g = std::min(static_cast<int>(a.g) + static_cast<int>(b.g), 255);
    sf::Uint8 b_ = std::min(static_cast<int>(a.b) + static_cast<int>(b.b), 255);
    return sf::Color(r, g, b_);
}

// Función auxiliar para mezclar dos colores promediando sus componentes RGB
sf::Color mixColorsAverage(const sf::Color& a, const sf::Color& b) {
    sf::Uint8 r = std::min((static_cast<int>(a.r) + static_cast<int>(b.r)) / 2, 255);
    sf::Uint8 g = std::min((static_cast<int>(a.g) + static_cast<int>(b.g)) / 2, 255);
    sf::Uint8 b_ = std::min((static_cast<int>(a.b) + static_cast<int>(b.b)) / 2, 255);
    return sf::Color(r, g, b_);
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
