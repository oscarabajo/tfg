# Definir el compilador y las opciones
CXX = g++
CXXFLAGS = -std=c++17 -Wall -g
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system -lrtmidi -pthread


# Archivos
SRCS = plotFormaEnPista.cpp ../colorFunctions/colorFunctions.cpp
OBJS = plotFormaEnPista.o ../colorFunctions/colorFunctions.o
EXEC = midiviewer

# Regla principal
all: $(EXEC)

# Regla para enlazar el ejecutable
$(EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Reglas para compilar los objetos
plotFormaEnPista.o: plotFormaEnPista.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

../colorFunctions/colorFunctions.o: ../colorFunctions/colorFunctions.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpiar archivos compilados
clean:
	rm -f $(OBJS) $(EXEC)

# Regla run: compila y ejecuta el programa
# Regla run: compila y ejecuta el programa usando parámetros desde el script
run: $(EXEC)
	./$(EXEC) "./decoder/$(CRIM_FILE).crim2s" $(BPM) $(MIX_STRATEGY)