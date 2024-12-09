#ifndef COLORFUNCTIONS_H
#define COLORFUNCTIONS_H

#include <SFML/Graphics.hpp>
#include <vector>

// Declaración de funciones
sf::Color setColorByOctaveLinealAbss2(int note);

// Funciones auxiliares para mezclar dos colores
sf::Color mixColorsSum(const sf::Color& a, const sf::Color& b);
sf::Color mixColorsAverage(const sf::Color& a, const sf::Color& b);

// Función para aplicar una estrategia de mezcla a múltiples colores
sf::Color applyMixingStrategy(const std::vector<sf::Color>& colors, sf::Color (*mixFunc)(const sf::Color&, const sf::Color&));

#endif // COLORFUNCTIONS_H
