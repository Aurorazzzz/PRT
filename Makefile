CC = gcc
CFLAGS = -O2 -Wall

SRC = script_principal.c \
      surveillance_temperature.c \
      surveillance_tension_Theo.c \
      Read_Write.c

# Chemin de sortie
OUTDIR = output

# Nom de l'exécutable final dans output/
TARGET = $(OUTDIR)/script_principal.exe

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
	rm -f *.o
