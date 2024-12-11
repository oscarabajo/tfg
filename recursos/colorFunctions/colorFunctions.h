// colorFunctions.h

#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

// Asigna un color a la nota basado en la octava y la escala de colores
sf::Color setColorByOctave(int note);
sf::Color setColorByOctaveBlue(int note);

// Función auxiliar para mezclar dos colores sumando sus componentes RGB
sf::Color mixColorsSum(const sf::Color& a, const sf::Color& b);

// Función auxiliar para mezclar dos colores promediando sus componentes RGB
sf::Color mixColorsAverage(const sf::Color& a, const sf::Color& b);

// Implementación de applyMixingStrategy
sf::Color applyMixingStrategy(const std::vector<sf::Color>& colors, sf::Color (*mixFunc)(const sf::Color&, const sf::Color&));
