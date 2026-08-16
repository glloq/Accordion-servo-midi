# Tests natifs : se compilent avec un simple g++, sans toolchain AVR ni acces reseau.
#
#   make        -> compile et execute toute la suite
#   make clean
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
	$(SRC_DIR)/bellowController.cpp \
	$(SRC_DIR)/instrument.cpp \
	$(SRC_DIR)/midiHandler.cpp

STUB_SRC = stubs/stubs.cpp stubs/stubs_io.cpp

BINARIES = \
	$(BUILD_DIR)/test_note_mapping \
	$(BUILD_DIR)/test_stepper_stop \
	$(BUILD_DIR)/test_timing \
	$(BUILD_DIR)/test_behavior \
	$(BUILD_DIR)/test_behavior_progmem

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

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
