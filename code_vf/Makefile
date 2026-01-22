CC = gcc
CFLAGS = -O2 -Wall -Iheaders
LDLIBS += -lm

SRC = sources/script_principal_candence.c \
      sources/sur_temperature.c \
      sources/sur_tension.c \
      sources/SOE.c \
      sources/SOH.c \
      sources/RUL.c \
      sources/RINT.c \
      sources/Read_Write.c \
      sources/SOC.c \
      sources/SOP.c \
      sources/etat_charge.c

# Chemin de sortie
OUTDIR = output

# Nom de l'exécutable final dans output/
TARGET = $(OUTDIR)/script_principal_candence.exe

all: $(TARGET)


$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDLIBS)

clean:
	rm -f $(TARGET)
	rm -f *.o
