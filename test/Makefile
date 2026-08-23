# Tests natifs : se compilent avec un simple g++, sans toolchain AVR ni acces reseau.
#
#   make        -> compile et execute toute la suite
#   make clean
#
# La suite couvre CHAQUE source d'air et chaque variante de configuration structurante :
# un firmware ou seule la configuration par defaut est testee n'a de modularite que le nom.
#
# Les bibliotheques Arduino sont remplacees par les stubs de test/stubs/.

CXX      ?= g++
CXXFLAGS ?= -std=gnu++11 -O1 -Wall -Wextra -Wno-unused-parameter
SRC_DIR   = ../accordionV06
BUILD_DIR = build

INCLUDES = -I. -Istubs -I$(SRC_DIR)

FIRMWARE_SRC = \
	$(SRC_DIR)/noteMapping.cpp \
	$(SRC_DIR)/servoController.cpp \
	$(SRC_DIR)/handController.cpp \
	$(SRC_DIR)/airValve.cpp \
	$(SRC_DIR)/pressureRegulator.cpp \
	$(SRC_DIR)/bellowController.cpp \
	$(SRC_DIR)/servoBellowController.cpp \
	$(SRC_DIR)/blowerController.cpp \
	$(SRC_DIR)/instrument.cpp \
	$(SRC_DIR)/midiHandler.cpp

STUB_SRC = stubs/stubs.cpp stubs/stubs_io.cpp

# Une source d'air = un binaire. Le choix se fait a la compilation dans le firmware :
# le tester autrement reviendrait a tester du code qui n'est jamais embarque.
AIR_SOURCE_VARIANTS = servo_bellow blower_pwm blower_esc pump

BINARIES = \
	$(BUILD_DIR)/test_note_mapping \
	$(BUILD_DIR)/test_note_mapping_servo_bellow \
	$(BUILD_DIR)/test_note_mapping_esc \
	$(BUILD_DIR)/test_stepper_stop \
	$(BUILD_DIR)/test_timing \
	$(BUILD_DIR)/test_behavior \
	$(BUILD_DIR)/test_behavior_progmem \
	$(BUILD_DIR)/test_pressure \
	$(addprefix $(BUILD_DIR)/test_air_,$(AIR_SOURCE_VARIANTS)) \
	$(BUILD_DIR)/test_variant_split \
	$(BUILD_DIR)/test_variant_merge \
	$(BUILD_DIR)/test_variant_solenoid

.PHONY: all test clean
all: test

test: $(BINARIES)
	@echo
	@for t in $(BINARIES); do \
		echo "### $$t"; \
		$$t || exit 1; \
	done
	@echo "Tous les tests sont passes."

# test_note_mapping ne depend que de noteMapping.h/.cpp (aucun stub necessaire)
$(BUILD_DIR)/test_note_mapping: test_note_mapping.cpp $(SRC_DIR)/noteMapping.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I. -I$(SRC_DIR) $^ -o $@

# Rejoue la verification des canaux avec les organes qui n'existent que dans d'autres
# configurations : servo de soufflet et ESC occupent eux aussi un canal PCA.
$(BUILD_DIR)/test_note_mapping_servo_bellow: test_note_mapping.cpp $(SRC_DIR)/noteMapping.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DAIR_SOURCE=AIR_SOURCE_BELLOW_SERVO -I. -I$(SRC_DIR) $^ -o $@

$(BUILD_DIR)/test_note_mapping_esc: test_note_mapping.cpp $(SRC_DIR)/noteMapping.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DAIR_SOURCE=AIR_SOURCE_BLOWER_ESC -I. -I$(SRC_DIR) $^ -o $@

$(BUILD_DIR)/test_stepper_stop: test_stepper_stop.cpp $(STUB_SRC) $(FIRMWARE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

$(BUILD_DIR)/test_timing: test_timing.cpp $(STUB_SRC) $(FIRMWARE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

$(BUILD_DIR)/test_behavior: test_behavior.cpp $(STUB_SRC) $(FIRMWARE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# Rejoue toute la suite de comportement en compilant le CHEMIN DE CODE AVR (tables en
# PROGMEM, acces via pgm_read_*). Verifie qu'aucun acces direct aux tables n'a survecu.
$(BUILD_DIR)/test_behavior_progmem: test_behavior.cpp $(STUB_SRC) $(FIRMWARE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DARDUINO_ARCH_AVR $(INCLUDES) $^ -o $@

# === SOURCES D'AIR ===
# Meme fichier de test, une definition de AIR_SOURCE differente par binaire.
$(BUILD_DIR)/test_air_servo_bellow: test_air_sources.cpp $(STUB_SRC) $(FIRMWARE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DAIR_SOURCE=AIR_SOURCE_BELLOW_SERVO $(INCLUDES) $^ -o $@

$(BUILD_DIR)/test_air_blower_pwm: test_air_sources.cpp $(STUB_SRC) $(FIRMWARE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DAIR_SOURCE=AIR_SOURCE_BLOWER_PWM $(INCLUDES) $^ -o $@

$(BUILD_DIR)/test_air_blower_esc: test_air_sources.cpp $(STUB_SRC) $(FIRMWARE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DAIR_SOURCE=AIR_SOURCE_BLOWER_ESC $(INCLUDES) $^ -o $@

$(BUILD_DIR)/test_air_pump: test_air_sources.cpp $(STUB_SRC) $(FIRMWARE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DAIR_SOURCE=AIR_SOURCE_PUMP_ONOFF $(INCLUDES) $^ -o $@

# === AUTRES VARIANTES DE CONFIGURATION ===
# Routage MIDI et type d'actionneur changent du code compile conditionnellement.
$(BUILD_DIR)/test_variant_split: test_variants.cpp $(STUB_SRC) $(FIRMWARE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DMIDI_ROUTING=MIDI_ROUTING_SPLIT $(INCLUDES) $^ -o $@

$(BUILD_DIR)/test_variant_merge: test_variants.cpp $(STUB_SRC) $(FIRMWARE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DMIDI_ROUTING=MIDI_ROUTING_MERGE $(INCLUDES) $^ -o $@

$(BUILD_DIR)/test_variant_solenoid: test_variants.cpp $(STUB_SRC) $(FIRMWARE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DNOTE_ACTUATOR=ACTUATOR_SOLENOID $(INCLUDES) $^ -o $@

# === REGULATION DE PRESSION ===
# Le capteur n'est pas monte sur la machine par defaut : le test l'active explicitement.
$(BUILD_DIR)/test_pressure: test_pressure.cpp $(STUB_SRC) $(FIRMWARE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DPRESSURE_SENSOR_ENABLED=1 $(INCLUDES) $^ -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
