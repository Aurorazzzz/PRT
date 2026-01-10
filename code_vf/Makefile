CC = gcc
CFLAGS = -O2 -Wall
LDLIBS += -lm

SRC = script_principal_candence.c \
      sur_temperature.c \
      sur_tension.c \
	  SOE.c \
	  SOH.c \
	  RUL.c \
	  RINT.c \
	  Read_Write.c \
	  SOC.c \
	  SOP.c \
	  etat_charge.c
      

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
